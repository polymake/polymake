#  Copyright (c) 1997-2020
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

package Polymake;

# an identifier (alphanumeric, first character not a digit)
declare $id_re = qr{(?> (?!\d)\w+ )}x;

# a property name (alphanumeric, first character is a letter)
declare $prop_name_re = qr{(?> [a-zA-Z]\w* )}x;

# a list of identifiers (separated by commas)
declare $ids_re = qr{(?> $id_re (?: \s*,\s* $id_re)* )}xo;

# a property name with optional attributes
declare $prop_name_attrs_re = qr{(?> $prop_name_re (?: \( \s* $ids_re \s* \) )? )}xo;

# a hierarchical identifier (identifiers connected with dots)
declare $hier_id_re = qr{(?> $id_re (?: \. $id_re)* )}xo;

# property path, property names connected with dots
declare $prop_path_re = qr{(?> $prop_name_re (?: \. $prop_name_re)* )}xo;

# a list of hierarchical identifiers (separated by commas)
declare $hier_ids_re = qr{(?> $hier_id_re (?: \s*,\s* $hier_id_re)* )}xo;

# list of alternative property paths
declare $prop_path_alt_re = qr{(?> $prop_path_re (?: \s*\|\s* $prop_path_re)* )}xo;

# property path with optional attributes
declare $prop_path_attrs_re = qr{(?> $prop_name_attrs_re (?: \. $prop_name_attrs_re)* )}xo;

# a lone identifier
declare $id_only_re = qr{ ($id_re) \s*$ }xo;

# fully qualified name
declare $qual_id_re = qr{(?> $id_re (?: :: $id_re)* )}xo;

# a list of fully qualified names
declare $qual_ids_re = qr{(?> $qual_id_re (?: \s*,\s* $qual_id_re)* )}xo;

# unqualified name
declare $unqual_id_re=qr{(?<!::) (?<!\w) $id_re (?! :: )}xo;

# a single or double quote
declare $anon_quote_re=qr{ ['"] }x;

# a single or double quote in a named capturing group
declare $quote_re=qr{ (?'quote' $anon_quote_re) }xo;

# anything but a quote character
declare $non_quote_re=qr{ [^"'] }x;

# anything but a quote character or white space
declare $non_quote_space_re=qr{ [^"'\s] }x;

# a sequence of symbols not being delimiters (paired brackets or quotes), escaped delimiters, or arrows
my $non_delims=qr{(?: [^()\[\]{}<>'"\#]++
                    | (?<=\\)(?<!\\\\) [()\[\]{}<>"'\#]
                    | (?<=[-=])(?<!<=) >
                   )*+ }x;

declare $single_quoted_re=qr{(?: [^']+ | (?<=\\)(?<!\\\\) ' )*+}x;
declare $double_quoted_re=qr{(?: [^"]+ | (?<=\\)(?<!\\\\) " )*+}x;

# a sequence in quotes
declare $anon_quoted_re=qr{(?: ' $single_quoted_re ' | " $double_quoted_re " )}xo;

# a sequence in quotes, capturing the contents
declare $quoted_re=qr{(?: ' (?'quoted' $single_quoted_re) ' |
                          " (?'quoted' $double_quoted_re) " )}xo;

# an expression in parentheses, braces, brackets, or quotes
# 1 capturing group
declare $confined_re=qr{ ( \( $non_delims (?: (?-1) $non_delims )* \) |
                           \[ $non_delims (?: (?-1) $non_delims )* \] |
                           \{ $non_delims (?: (?-1) $non_delims )* \} |
                            < $non_delims (?: (?-1) $non_delims )*  > |
                            $anon_quoted_re                            ) }xo;

# a piece of code with proper nested embraced and quoted subexpressions
# 1 capturing group
declare $balanced_re=qr{ $non_delims (?: $confined_re $non_delims )* }xo;

# as above, but allowing some unmatched open braces
# used in TAB completion
declare $open_balanced_re=qr{ (?: $non_delims (?: \( (?: $balanced_re \) )?+ |
                                                  \[ (?: $balanced_re \] )?+ |
                                                  \{ (?: $balanced_re \} )?+ |
                                                   < (?: $balanced_re  > )?+ |
                                                   $anon_quoted_re           |
                                                   $ ) )* }xo;

# a piece of code with all quoted strings properly closed
declare $quote_balanced_re=qr{ $non_quote_re* (?: $anon_quoted_re $non_quote_re* )* }xo;

# parameter list in angle brackets, recursively referring to the outer group
# this is a fragment of a regex, referring to a group outside this string, therefore not defined as qr{}
my $param_list='(?: \s*<\s* (?-1) (?: \s*,\s* (?-1) )* \s*> )?+';

# property type, possibly parameterized
# 1 capturing group
declare $type_re=qr{($qual_id_re $param_list)}xo;

# a list of types (separated by commas)
# 1 capturing group
declare $types_re=qr{ $type_re (?: \s*,\s* (?-1) )*+ }xo;

# a type expression qualifying some following name
# (can't simply write $type_re :: $hier_id_re here because of the greedy nature of the former)
# 1 capturing group
declare $type_qual_re=qr{ $id_re (?: :: $id_re)* (?: \s*<\s* $types_re \s*>\s* )? (?= ::) }xo;

# an expression constructing a type: either a type name, optionally qualified and parameterized, or a piece of code
# 4 capturing groups
declare $type_expr_re=qr{ (?: (?= [\(\{]) (?'dynamic' $confined_re) | (?'static' $type_re)) }xo;

# a list of expressions constructing types (separated by commas)
# 5 capturing groups
declare $type_exprs_re=qr{ ($type_expr_re (?: \s*,\s* (?-5))*+) }xo;

# a type parameter in a declaration of a complex type or a function
# 6 capturing groups
declare $type_param_re=qr{ (?'name' $id_re) (?: \s*=\s* (?'default' $type_expr_re) )?+ }xo;

# list of type parameters in angle brackets with optional default values
# 7 capturing groups
declare $type_params_re=qr{ \s*<\s* (?'tparams' ($type_param_re (?: \s*,\s* (?-7) )*+ )) \s*> }xo;

# list of type parameters in angle brackets with optional default values
# 7 capturing groups
declare $type_params_variadic_re=qr{ \s*<\s* (?'tparams' ($type_param_re (?: \s*,\s* (?-7) )*+ ))
                                             (?('default') | (?: \s* (?'ellipsis' \.\.\.))? ) \s*> }xo;

# type-checking expressions
declare $typechecks_re=qr{ \s*\[ (?'typecheck' $balanced_re) \] }xo;

# beginning of a declaration of a type or function
declare $parametrized_decl_re=qr{ (?'lead_name' $id_re) (?: $type_params_re (?: $typechecks_re )?+ )?+ }xo;

# function declaration with optional signature
declare $sub_re=qr{ (?'name' $id_re) \b (?: \s*\( (?'signature' $balanced_re) \) )? }xo;

# function declaration with optional template parameters and signature
declare $parametrized_sub_re=qr{ $parametrized_decl_re (?: \s*\( (?'signature' $balanced_re) \) )?+ (?= [\s:;\{]) }xo;

# overloaded function declaration with optional labels
declare $labeled_sub_re=qr{ (?:(?'label' $hier_id_re) \s*:\s*)? $parametrized_sub_re }xo;

# attributes of an argument in a function signature, maybe empty
declare $func_arg_attrs_re=qr{(?=\s*[:&]) (?> (?: \s*:\s* $id_re )* (?: \s* &&? (?: \s* const\b)?)? )}xo;

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
declare $nonsignificant_line_re=qr{^ [ \t]* (?:\#.*)? $}xm;

# a line with some contents (like perl code)
declare $significant_line_re=qr{^ [ \t]* (?! $ | \#) }xm;

# beginning of a complete statement
declare $statement_start_re=qr{(?: ^ | [;\}] )\s*}x;

# beginning of a function argument list
declare $args_start_re=qr{(?'args_start' \s+ | \s*\(\s* )}x;

# an expression in a list, like an argument in a function call
# This pattern tries hard to recognize commas within type expressions,
# while all other < and > signs being interpreted as comparsion operators
declare $expression_re=qr{ (?: (?! <) $confined_re | (?> \$$qual_id_re | $type_re ) | [^,'"()\[\]{}]*+ )+ }xo;

# end of a source file
declare $end_of_source_file="__END__";
declare $end_of_source_file_re=qr/^${end_of_source_file}$/o;

sub trim_spaces {
   my ($string)=@_;
   $string =~ s/^\s+//;
   $string =~ s/\s+$//;
   $string
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
