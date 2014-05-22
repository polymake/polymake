#  Copyright (c) 1997-2014
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

use namespaces;

package Polymake;

# an identifier (alphanumeric, first character not a digit)
declare $id_re=qr{(?> (?!\d)\w+ )}x;

# a list of identifiers (separated by commas)
declare $ids_re=qr{(?> $id_re (?: \s*,\s* $id_re)* )}xo;

# hierarchical identifier (identifiers connected with dots)
declare $hier_id_re=qr{(?> $id_re (?: \. $id_re)* )}xo;

# a list of hierarchical identifiers (separated by commas)
declare $hier_ids_re=qr{(?> $hier_id_re (?: \s*,\s* $hier_id_re)* )}xo;

# a list of hierarchical identifiers (separated by bars)
declare $hier_id_alt_re=qr{(?> $hier_id_re (?: \s*\|\s* $hier_id_re)* )}xo;

# a list of alternatives (separated by commas)
declare $hier_id_alts_re=qr{(?> $hier_id_alt_re (?: \s*,\s* $hier_id_alt_re)* )}xo;

# a lone identifier
declare $id_only_re=qr{ ($id_re) \s*$ }xo;

# fully qualified name
declare $qual_id_re=qr{(?> $id_re (?: :: $id_re)* )}xo;

# a list of fully qualified names
declare $qual_ids_re=qr{(?> $qual_id_re (?: \s*,\s* $qual_id_re)* )}xo;

# unqualified name
declare $unqual_id_re=qr{(?<!::) (?<!\w) $id_re (?! :: )}xo;

# a sequence of symbols not being delimiters (paired brackets or quotes), escaped delimiters, or arrows
my $non_delims=qr{(?: [^()\[\]{}<>'"\#]++
                    | (?<=\\)(?<!\\\\) [()\[\]{}<>"'\#]
                    | (?<=[-=])(?<!<=) >
                   )*+ }x;

declare $single_quoted_re=qr{(?: [^']+ | (?<=\\)(?<!\\\\) ' )*+}x;
declare $double_quoted_re=qr{(?: [^"]+ | (?<=\\)(?<!\\\\) " )*+}x;

declare $confined_re=qr{ ( \( $non_delims (?: (?-1) $non_delims )* \) |
                           \[ $non_delims (?: (?-1) $non_delims )* \] |
                           \{ $non_delims (?: (?-1) $non_delims )* \} |
                            < $non_delims (?: (?-1) $non_delims )*  > |
                            ' $single_quoted_re '                     |
                            " $double_quoted_re "                      ) }xo;

declare $balanced_re=qr{ $non_delims (?: $confined_re $non_delims )* }xo;

# allow some unmatched open braces
declare $open_balanced_re=qr{ (?: $non_delims (?: \( (?: $balanced_re \) )?+ |
                                                  \[ (?: $balanced_re \] )?+ |
                                                  \{ (?: $balanced_re \} )?+ |
                                                   < (?: $balanced_re  > )?+ |
                                                   ' $single_quoted_re '     |
                                                   " $double_quoted_re "     |
                                                   $ ) )* }xo;

# an expression in a list; < > are treated as comparison ops, therefore not trying to match them pairwise
declare $expression_re=qr{ (?: (?! <) $confined_re | [^,'"()\[\]{}]*+ )+ }xo;

# parameter list in angle brackets, recursively referring to the outer group
# this is a fragment of a regex, referring to a group outside this string, therefore not defined as qr{}
declare $param_list='(?: \s*<\s* (?-1) (?: \s*,\s* (?-1) )* \s*> )?+';

# property type, possibly parameterized
declare $type_re=qr{($qual_id_re $param_list)}xo;

# a list of types (separated by commas)
declare $types_re=qr{ $type_re (?: \s*,\s* (?-1) )*+ }xo;   # type_re contains one capturing group

# a list of alternative types (separated by bars)
declare $type_alt_re=qr{ $type_re (?: \s*\|\s* (?-1))*+ }xo;

# a type expression qualifying some following name
# (can't simply write $type_re :: $hier_id_re here because of the greedy nature of the former)
declare $type_qual_re=qr{ $id_re (?: :: $id_re)* (?: \s*<\s* $types_re \s*>\s* )? (?= ::) }xo;

# an expression constructing a type: either a type name, optionally qualified and parameterized, or a piece of code
declare $type_expr_re=qr{ (?: $type_re | \{ $balanced_re \} ) }xo;

# a type parameter in a declaration of a complex type or a function
declare $type_param_re=qr{ (?'name' $id_re) (?: \s*=\s* (?'default' $type_expr_re) )?+ }xo;

# a list of type parameters, enclosed in angle brackets, with optional typecheck expression
declare $type_params_re=qr{ \s*<\s* (?'tparams' ($type_param_re (?: \s*,\s* (?-5) )*+ )) \s*>    # type_param_re contains 4 capturing groups
                            (?: \s*\[ (?'typecheck' $balanced_re) \] )?+ }xo;

# beginning of a declaration of a type or function, optionally parameterized
declare $paramed_decl_re=qr{ (?'lead_name' $id_re) (?: $type_params_re )?+ }xo;

# function declaration with optional signature
declare $sub_re=qr{ (?'name' $id_re) \b (?: \s*\( (?'signature' $balanced_re) \) )? }xo;

# function declaration with optional template parameters and signature
declare $paramed_sub_re=qr{ $paramed_decl_re (?: \s*\( (?'signature' $balanced_re) \) )?+ (?= [\s:;\{]) }xo;

# overloaded function declaration with optional labels
declare $labeled_sub_re=qr{ (?:(?'labels' $hier_ids_re) \s*:\s*)? $paramed_sub_re }xo;

# rule input clause
declare $rule_input_re=qr{ $hier_id_re (?: \s*[|,]\s* $hier_id_re )* }xo;

# filename (directory part stripped)
declare $filename_re=qr{ ([^/]+) $ }x;

# directory name (file part stripped)
declare $directory_re=qr{ ^(.*) / [^/]+$ }x;

# directory part of a command, expecting trailing arguments
declare $directory_of_cmd_re=qr{ ^(.*) / [^/]+ (?: $ | \s)}x;

# an empty line
declare $empty_line_re=qr{^ [ \t]* \n}xm;

# an empty line or separator (a line of hashes)
declare $empty_or_separator_line=qr{^ (?: [ \t]* | \#{2,} ) \n}xm;

# an empty line with possible comments
declare $nonsignificant_line_re=qr{^ [ \t]* (?:\#.*)? \n}xm;

# a line with some contents (like perl code)
declare $significant_line_re=qr{^ [ \t]* (?! $ | \#) }xm;

# Transform a parameterized type into a valid perl expression:
# "NAME<PARAM1,PARAM2>" => "NAME->type(PARAM1->type,PARAM2->type)"
# References to arguments $_[NN] are preserved.
sub translate_type {
   $_[0] =~ tr/<>/()/;
   substr($_[0], rindex($_[0],";")+1) =~ s{($id_re) (?! \s*(?: [-:[] | => ))}{$1->type}xog;
}

# Transform a complex expression involving parameterized types into a valid perl expression.
sub translate_type_expr {
   $_[0] =~ s{ $type_re (?! \s*(?: [([] | -> | => )) }{ my $tt=$1; translate_type($tt); $tt }xoge;
}

sub report_type_error {
   if ($@ =~ /^Undefined subroutine &?(?:$id_re\::)*($id_re) called/o ||
       $@ =~ /^Bareword "($id_re)" not allowed/o ||
       $@ =~ /Package "($id_re)" does not exist/o) {
      croak( "Unknown type '$1'" );
   } elsif ($@ =~ s/ at \(eval \d+\).*\n//) {
      croak( "Invalid type expression '$_[0]': $@" );
   } else {
      die $@;
   }
}

sub eval_type_expr {
   my $expr=shift;
   if (is_string($$expr)) {
      translate_type($$expr);
      $$expr=eval($$expr) || report_type_error($$expr);
   } else {
      $$expr;
   }
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
