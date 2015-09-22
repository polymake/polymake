#  Copyright (c) 1997-2015
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

use strict;
use namespaces;

# Analyzers of partial input, supporting TAB completion and F1 context-sensitive help

package Polymake::Core::Shell::Helper;


use Polymake::Struct (
   [ new => '$$$;$' ],
   [ '$name' => '#1' ],         # descriptive name

# TODO: remove this hack when support for perl 5.10 is phased out.
# Before perl 5.12, compiled qr// patterns were not attached to references.

   [ '$pattern' => ($^V lt v5.12 ? '\\( #2 )' : '#2') ],      # pattern to match the partial input

   [ '&completion' => '#3' ],   # ($Shell, $word) => 1 for `ready' || 0 or undef for `try next pattern'
   [ '&help' => '#4' ],         # ($Shell) => (start position in partial input, InteractiveHelp topics) || () for `try next pattern'
);

# the term is NOT a method call, if preceded by:
my $op_re=qr{(?:[-+*/.,<=;({}\[]| (?<!-)> | ^ )\s* | \w\s+ }x;

# a variable or array element
my $var_re=qr{ \$ $id_re (?:(?=\s*\[) $confined_re)? }xo;

# a function name, possibly qualified
my $func_name_re=qr{(?:(?'app_name' $id_re)::)?(?'func' $id_re)}xo;

# start of a method call chain: a variable or a function call
my $var_or_func_re=qr{ (?'var' $var_re) | $func_name_re (?(?=\s*\() $confined_re)? }xo;

# intermediate expression in a method call chain
my $intermed_re=qr{ \s*->\s* (?: (?'method_name' $id_re) (?(?=\s*\() $confined_re)? | (?(?=\s*\[) $confined_re ) ) }xo;

# chain of intermediate expressions
my $intermed_chain_re=qr{ (?'intermed' (?:$intermed_re)*?) \s*->\s* }xo;

# beginning of a function argument list
my $args_start_re=qr{(?'args_start' \s+ | \s*\(\s* )}x;

# beginning of a method argument list
my $meth_args_start_re=qr{(?'args_start' \s*\(\s* )}x;

my (%popular_perl_keywords, %object_methods);
@popular_perl_keywords{qw( local print require chdir rename unlink mkdir rmdir declare exit )}=();
@object_methods{qw( name description give take add remove type provide lookup list_properties properties schedule list_names
                    get_attachment remove_attachment list_attachments attach )}=();


# The order is important: more specific patterns must precede more generic ones.
# The analysis is performed until the first successful match.

declare @list=(

   new Helper(
      'filename argument',

      qr{(?: $statement_start_re (?: (?'simple_cmd' require | do | rename | unlink | mkdir | load_commands | save_history | export_configured)
                                   | (?'dir_cmd' chdir | rmdir | found_extension | import_extension)) $args_start_re
           | $op_re (?: load | load_data | glob ) $args_start_re
           | $statement_start_re (?: save | save_data ) $args_start_re $expression_re ,\s*
           | \b(?'File' File) \s*=>\s* )
         (?: $quote_re (?'prefix' $non_quote_re*)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($simple_cmd, $dir_cmd, $args_start, $File, $quote, $prefix)=@+{qw(simple_cmd dir_cmd args_start File quote prefix)};

         if (defined $quote) {
            $shell->try_filename_completion($word, $quote, $prefix, $dir_cmd);
         } else {
            $shell->completion_words=[ $simple_cmd || $dir_cmd || $args_start =~ /\(/ || $File ? '"' : '(' ];
         }
         1
      },

      sub {
         # no sensible help on the filename, proceed to the function/method name
         (capturing_group_boundaries(defined $+{File} ? 'File' : 'args_start'))[0];
      }
   ),

   new Helper(
      'application name argument',

      qr{ $statement_start_re (extend_)?application $args_start_re (?(1) (?'ext_dir' $expression_re) ,\s*)
          (?: $quote_re (?'prefix' $non_quote_re*)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($ext_dir, $quote, $prefix)=@+{qw(ext_dir quote prefix)};

         if (defined $quote) {
            $shell->term->Attribs->{completion_append_character}=$quote;
            if ($ext_dir) {
               $ext_dir=eval $ext_dir or return 1;
               replace_special_paths($ext_dir);
               if ($ext_dir !~ m{^/} && defined (my $ext=$Extension::registered_by_URI{$ext_dir})) {
                  $ext_dir=$ext->dir;
              }
           }
           $shell->completion_words=[ sorted_uniq(sort( grep { not $ext_dir && -d "$ext_dir/apps/$_" }
                                                        map { $_ =~ $filename_re }
                                                        map { glob "$_/apps/$prefix*" }
                                                        $InstallTop, map { $_->dir } @Extension::active[$Extension::num_bundled .. $#Extension::active] )) ];
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my $app_name=$+{prefix};
         if (length($app_name) > 0 and
             defined(my $app=eval { User::application($app_name) })) {
            ($start_pos, $app->help);
         } else {
            # proceed with the command itself
            $start_pos;
         }
      }
   ),

   new Helper(
      'label argument for preference manipulations',

      qr{ $statement_start_re (?'cmd' prefer(?:_now)? | (?:re)?set_preference ) $args_start_re
          (?: $quote_re (?'prefix' (?'app_name' $id_re) ::)?
              (?'expr' \*\.(?: $hier_id_re (?: \s+ (?: $ids_re (?:\s*,)? )? | \.? )? )? | $hier_id_re\.?)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($cmd, $quote, $prefix, $app_name, $expr)=@+{qw(cmd quote prefix app_name expr)};

         if (defined $quote) {
            my @proposals;
            if (defined $app_name) {
               if (defined (my $app=eval { User::application($app_name) })) {
                  @proposals=map { "$app_name\::$_" } try_label_completion($app, $expr);
               }
            } else {
               @proposals=try_label_completion($User::application, $expr);
               if (length($expr)==0 || $expr =~ /^$id_re$/o) {
                  push @proposals, map { "$_\::" } grep { /^$expr/ && !$User::application->imported->{$_} } keys %{$User::application->used};
               }
            }
            finalize_proposals($shell, $prefix, $word, @proposals);

            if (@{$shell->completion_words}==1 && $shell->completion_words->[0] !~ /:$/) {
               $shell->term->Attribs->{completion_append_character}=
                  $cmd ne "reset_preference" && $expr =~ /^\*\.$hier_id_re\s*$/o
                  ? " " : $quote;
            }
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my ($app_name, $expr)=@+{qw(app_name expr)};
         my @proposals;
         if ($expr =~ /($id_re) (?: \. $id_re)* \.? \s* $/xo) {
            my $label_name=$1;
            if (defined (my $app= defined($app_name) ? eval { User::application($app_name) } : $User::application)) {
               @proposals=$app->help->find('preferences', $label_name);
            }
         }
         # afterwards proceed with the command itself
         ($start_pos, @proposals)
      }
   ),

   new Helper(
      'extension configuration option',

      qr{ $statement_start_re (?:import|reconfigure)_extension $args_start_re (?'ext_dir' $expression_re) ,\s*
          (?: $expression_re ,\s* )* $quote_re? (?'prefix' $non_quote_space_re*) $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($quote, $prefix)=@+{qw(quote prefix)};
         my $ext_dir=eval($+{ext_dir});
         if (defined ($ext_dir) &&
             defined ($ext_dir=verify_ext_dir($ext_dir)) and
             -f "$ext_dir/configure.pl") {
            if (defined $quote) {
               require Symbol;
               my (%allowed_options, %allowed_with, $err);
               {  package Polymake::StandaloneExt;
                  do "$ext_dir/configure.pl";
                  $err=$@;
               }
               unless ($err) {
                  eval "Polymake::StandaloneExt::allowed_options(\\%allowed_options, \\%allowed_with)";
                  $err=$@;
               }
               Symbol::delete_package("Polymake::StandaloneExt");
               return 1 if $err;
               finalize_proposals($shell, $prefix, $word,
                                  sort( grep { /^\Q$prefix\E/ }
                                        (map { "--$_" } keys %allowed_options),
                                        (map { ("--with-$_", "--without-$_") } keys %allowed_with) ));
               $shell->term->Attribs->{completion_append_character}=$quote;
            } else {
               $shell->completion_words=[ '"' ];
            }
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my $ext_dir=eval($+{ext_dir});
         if (defined ($ext_dir) &&
             defined ($ext_dir=verify_ext_dir($ext_dir)) and
             -f "$ext_dir/configure.pl") {
            return ($start_pos, sub {
               require Symbol;
               package Polymake::StandaloneExt;
               do "$ext_dir/configure.pl";
               unless ($@) {
                  print STDERR "\nFollowing configuration options are possible:\n";
                  eval { usage() };
               }
               Symbol::delete_package("Polymake::StandaloneExt");
            });
         }
         # proceed with the command itself
         $start_pos
      }
   ),

   new Helper(
      'extension directory or URI',

      qr{ $statement_start_re (?:(?:obliterate|reconfigure)_extension | (?'app_cmd' found|extend)_application)
          $args_start_re (?: $quote_re (?'prefix' $non_quote_re*) )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($app_cmd, $quote, $prefix)=@+{qw(app_cmd quote prefix)};

         if (defined $quote) {
            my $match= $prefix =~ m{^/} ? "dir" : "URI";
            my @proposals=map { my $item=$_->$match;
                                ($app_cmd eq "extend" || !$_->is_bundled) && $item =~ /^\Q$prefix\E/ ? ($item) : ()
                          } values %Extension::registered_by_dir;

            if ($app_cmd eq "found" && index("core", $prefix)==0) {
               push @proposals, "core";
            }
            finalize_proposals($shell, $prefix, $word, @proposals);
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         if (defined (my $ext=verify_extension($+{prefix}, $+{app_cmd} eq "extend"))) {
            if (defined($ext->credit) && length($ext->credit->text)>0) {
               return ($start_pos, sub {
                  print "Extension contains package ", $ext->credit->product, ":\n", $ext->credit->text;
               });
            }
         }
         # proceed with the command itself
         $start_pos
      }
   ),

   new Helper(
      'rulefile argument',

      qr{ $statement_start_re (?'cmd' reconfigure|unconfigure|include)
          $args_start_re (?: $quote_re (?'prefix' (?'app_name' $id_re) ::)? (?'filename' $non_quote_space_re*) )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($cmd, $quote, $prefix, $app_name, $filename)=@+{qw(cmd quote prefix app_name filename)};

         if (defined $quote) {
            my @proposals;
            if (defined $app_name) {
               if (defined (my $app=lookup Application($app_name))) {
                  @proposals=map { "$app_name\::$_" } matching_rulefiles($app, $cmd, $filename);
               }
            } else {
               @proposals=matching_rulefiles($User::application, $cmd, $filename);
               while (my ($app_name, $app)=each %{$User::application->used}) {
                  if ($app_name =~ /^\Q$filename\E/ and
                      $cmd eq "include" ? glob($app->rulepath->[0]."/*") : $app->list_configured($cmd eq "unconfigure")) {
                     push @proposals, "$app_name\::";
                  }
               }
            }
            if (@proposals==1 && $proposals[0] !~ /::$/) {
               $shell->term->Attribs->{completion_append_character}=$quote;
            }
            finalize_proposals($shell, $prefix, $word, sort @proposals);

         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         # nothing to tell about a rulefile, proceed with the command
         (capturing_group_boundaries('args_start'))[0]
      }
   ),

   new Helper(
      'script file argument',

      qr{ $op_re script $args_start_re (?: $quote_re (?'prefix' $non_quote_re*)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($args_start, $quote, $prefix)=@+{qw(args_start quote prefix)};

         if (defined $quote) {
            if ($prefix =~ m|^[./~]|) {
               $shell->try_filename_completion($word, $quote, $prefix, 0);
            } else {
               my @proposals=$shell->term->completion_matches($prefix, $shell->term->Attribs->{filename_completion_function});
               foreach my $dir ((map { @{$_->scriptpath} } $User::application, values %{$User::application->imported}),
                                @User::lookup_scripts, "$InstallTop/scripts") {
                  if (my @sublist=$shell->term->completion_matches("$dir/$prefix", $shell->term->Attribs->{filename_completion_function})) {
                     shift @sublist if $#sublist;
                     my $tail=length($dir)+1;
                     push @proposals, map { substr($_, $tail) } @sublist;
                  }
               }
               if (@proposals) {
                  $shell->postprocess_file_list($word, $quote, $prefix, 0, \@proposals);
               }
            }
         } else {
            $shell->completion_words=[ $args_start =~ /\(/ ? '"' : '(' ];
         }
         1
      },

      sub {
         # nothing to tell about a script file, proceed with the command
         (capturing_group_boundaries('args_start'))[0]
      }
   ),

   new Helper(
      'quoted property name argument',

      qr{ $op_re $var_or_func_re $intermed_chain_re
          (?: (?'mult_word' give | lookup | (?'mult_arg' provide | get_schedule | remove)) | take | add ) $meth_args_start_re
          (?('mult_arg') (?: ($anon_quote_re) $hier_id_alt_re \g{-1} \s*,\s*)* )
          (?: $quote_re (?('mult_word') (?: $hier_id_re \s*\|\s*)* ) (?'prefix' $hier_id_re\.?)?)? $}xo,

      sub {
         my ($shell, $word)=@_;
         if (defined $+{quote}) {
            my ($var, $app_name, $func, $intermed, $quote, $prefix)=@+{qw(var app_name func intermed quote prefix)};

            if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
               if (instanceof ObjectType($type)) {
                  $shell->completion_words=[ try_property_completion($type, $prefix, 1) ];
                  $shell->term->Attribs->{completion_append_character}=$quote;
               }
            }
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my ($var, $app_name, $func, $intermed, $prefix)=@+{qw(var app_name func intermed prefix)};
         my @proposals;
         if (length($prefix)) {
            if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
               if (instanceof ObjectType($type)) {
                  @proposals=fetch_property_help($type, $prefix, 1)
                    or
                  @proposals=sub { print "object type ", $type->full_name, " does not have a property $prefix\n" };
               }
            }
         }
         # afterwards proceed with the method itself
         ($start_pos, @proposals);
      }
   ),

   new Helper(
      'added multiple subobject property name',

      qr{ $op_re $var_or_func_re $intermed_chain_re
          add $meth_args_start_re ($anon_quote_re)(?'prop_name' $id_re)\g{-2} \s*,\s*
          (?'leading_args' (?: $expression_re ,\s* )?
             (?: (?: $id_re | ($anon_quote_re) $hier_id_re \g{-1} ) \s*=>\s* $expression_re ,\s* )* )
          $quote_re? (?'prefix' $hier_id_re\.?)? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($var, $app_name, $func, $intermed, $prop_name, $leading_args, $quote, $prefix)=@+{qw(var app_name func intermed prop_name leading_args quote prefix)};

         if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
            if (instanceof ObjectType($type) && defined(my $prop=$type->lookup_property($prop_name))) {
               $shell->completion_words=[ try_property_completion($prop->specialization($type)->type, $prefix) ];
               if ($quote) {
                  $shell->term->Attribs->{completion_append_character}=$quote;
               } else {
                  $_ .= "=>" for @{$shell->completion_words};
               }
               if (length($leading_args)==0 && !$quote && index("temporary", $prefix)==0) {
                  push @{$shell->completion_words}, "temporary,";
               }
            }
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my ($var, $app_name, $func, $intermed, $prop_name, $prefix)=@+{qw(var app_name func intermed prop_name prefix)};
         my @proposals;
         if (length($prefix) && $prefix ne "temporary") {
            if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
               if (instanceof ObjectType($type) && defined(my $prop=$type->lookup_property($prop_name))) {
                  $type=$prop->specialization($type)->type;
                  @proposals=fetch_property_help($type, $prefix)
                    or
                  @proposals=sub { print "object type ", $type->full_name, " does not have a property $prefix\n" };
               }
            }
         }
         # afterwards proceed with the method itself
         ($start_pos, @proposals);
      }
   ),

   new Helper(
      'attachment name argument',

      qr{ $op_re $var_or_func_re $intermed_chain_re (?: get_attachment | remove_attachment )
          $meth_args_start_re (?: $quote_re (?'prefix' $id_re)?)? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($var, $app_name, $func, $intermed, $quote, $prefix)=@+{qw(var app_name func intermed quote prefix)};

         if (defined $quote) {
            if (($var)=retrieve_method_owner_type($var, $app_name, $func, $intermed)) {
               if (is_object($var) && instanceof Core::Object($var)) {
                  $shell->completion_words=[ sort( grep { /^$prefix/ } keys %{$var->attachments} ) ];
                  $shell->term->Attribs->{completion_append_character}=$quote;
               }
            }
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         # nothing to tell about an attachment, proceed with the method
         (capturing_group_boundaries('args_start'))[0]
      }
   ),

   new Helper(
      'rule label or header argument of a method',

      qr{ $op_re $var_or_func_re $intermed_chain_re (?: disable_rules | apply_rule )
          $meth_args_start_re (?: $quote_re (?'prefix' $non_quote_re*))? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($var, $app_name, $func, $intermed, $quote, $prefix)=@+{qw(var app_name func intermed quote prefix)};

         if (defined $quote) {
            if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
               if (instanceof ObjectType($type)) {
                  my @proposals;
                  if ($prefix =~ /^ (?: $hier_id_re (?:\.)? )? $/xo) {
                     # maybe a label
                     @proposals=sorted_uniq(sort( grep { /^\Q$prefix\E/ } map { $_->full_name } map { @{$_->labels} } map { @{$_->production_rules} } $type, @{$type->super} ));
                  }
                  my $pattern=$prefix;
                  Rule::prepare_header_search_pattern($pattern);
                  push @proposals, sorted_uniq(sort( grep { /^$pattern/ } map { $_->header } map { @{$_->production_rules} } $type, @{$type->super} ));
                  finalize_proposals($shell, $prefix, $word, @proposals);
                  $shell->term->Attribs->{completion_append_character}=$quote;
               }
            }
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my ($var, $app_name, $func, $intermed, $prefix)=@+{qw(var app_name func intermed prefix)};
         my @proposals;
         if ($prefix =~ /($id_re) (?: \. $id_re)* \.? \s* $/xo) {
            my $label_name=$1;
            if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
               if (instanceof ObjectType($type)) {
                  @proposals=$type->application->help->find('preferences', $label_name);
               }
            }
         }
         # afterwards proceed with the method itself
         ($start_pos, @proposals)
      }
   ),

   new Helper(
      'rule label or header argument of a function',

      qr{ $statement_start_re disable_rules $args_start_re (?: $quote_re (?'prefix' $non_quote_re*))? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($quote, $prefix)=@+{qw(quote prefix)};

         if (defined $quote) {
            my @proposals;
            if ($prefix =~ /^ (?: $hier_id_re (?:\.)? )? $/xo) {
               # maybe a label
               @proposals=sorted_uniq(sort( grep { /^\Q$prefix\E/ } map { $_->full_name } map { @{$_->labels} } @{$User::application->rules} ));
            }
            my $pattern=$prefix;
            Rule::prepare_header_search_pattern($pattern);
            push @proposals, sorted_uniq(sort( grep { /^$pattern/ } map { $_->header } @{$User::application->rules} ));
            finalize_proposals($shell, $prefix, $word, @proposals);
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         my ($prefix)=@+{qw(prefix)};
         my @proposals;
         if ($prefix =~ /($id_re) (?: \. $id_re)* \.? \s* $/xo) {
            @proposals=$User::application->help->find('preferences', $1);
         }
         # afterwards proceed with the command itself
         ($start_pos, @proposals)
      }
   ),

   new Helper(
      'help topic',

      qr{ $statement_start_re help $args_start_re (?: $quote_re (?'path' $non_quote_re*/)? (?'prefix' $non_quote_re*))? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($quote, $path, $prefix)=@+{qw(quote path prefix)};

         if (defined $quote) {
            my $app_name;
            if ($path =~ s/^($id_re):://o) {
               $app_name=$1;
            } elsif (!defined($path) && $prefix =~ s/^($id_re):://o) {
               $app_name=$1;
            }
            my $app= defined($app_name) ? (eval { User::application($app_name) } // return) : $User::application;
            my @proposals= sorted_uniq(sort( defined($path)
                                             ? map { $_->list_toc_completions($prefix) } $app->help->get_topics($path) :
                                             defined($app_name)
                                             ? map { $app_name."::".$_ } $app->help->list_completions("!rel", $prefix)
                                             : $app->help->list_completions($prefix) ));
            if (@proposals==1 &&
                grep { @{$_->toc} } $app->help->get_topics($path.$proposals[0])) {
               $quote="/";
            }
            if (!defined($app_name) && !defined($path)) {
               $quote="" if !@proposals;
               push @proposals, map { $_."::" } grep { /^$prefix/ } known Application;
            }
            finalize_proposals($shell, $prefix, $word, @proposals);
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_words=[ "'" ];
         }
         1
      },

      sub {
         # nothing to tell about the argument, proceed with the command
         (capturing_group_boundaries('args_start'))[0]
      }
   ),

   new Helper(
      'qualified variable name',

      qr{ (?: $statement_start_re (?'cmd' local | (?:re)?set_custom) $args_start_re)?
          (?'type' (?'scalar'\$)|[\@%])
          (?: (?'varname' $qual_id_re(?'qual'::)?)
              (?('scalar') (?('qual') | \s*\{\s* (?'keyquote' ['"])?(?'key' [\w-]*) )?) )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($cmd, $type, $varname, $keyquote, $key)=@+{qw(cmd type varname keyquote key)};

         if (defined $key) {
            # completing a hash key
            my $i=0;
            if (my @custom_maps=grep { defined } map { $_->custom->find("%$varname", ++$i >= 3) }
                                     $User::application, $Prefs, values %{$User::application->imported}) {
               finalize_proposals($shell, $key, $word, sorted_uniq(sort( grep { index($_, $key)==0 } map { keys %{$_->default_value} } @custom_maps )));
            } elsif (defined (my $user_hash=find_user_variable('%', $varname))) {
               finalize_proposals($shell, $key, $word, sort( grep { index($_, $key)==0 } keys %$user_hash ));
            }

            if (@{$shell->completion_words}==1) {
               $shell->term->Attribs->{completion_append_character}=$keyquote || '}';
            }

         } else {
            # completing variable name
            my $i=0;
            my @proposals=map { $_->custom->list_completions($type, $varname, ++$i >= 3) }
                              $User::application, $Prefs, values %{$User::application->imported};
            if ($cmd eq "" || $cmd eq "local"
                  and
                !@proposals || $varname !~ /::/) {
               push @proposals, complete_user_variable_name($type, $varname);
            }
            $shell->completion_words=[ sort(@proposals) ];
            if ($cmd && @proposals==1 && substr($proposals[0], -1) ne ":") {
               $shell->term->Attribs->{completion_append_character}= $cmd eq "reset_custom" ? ";" : "=";
               if ($type eq '$') {
                  foreach $type (qw(% @)) {
                     $i=0;
                     if (grep { defined } map { $_->custom->find($type.$proposals[0], ++$i >= 3) }
                              $User::application, $Prefs, values %{$User::application->imported}) {
                        $shell->term->Attribs->{completion_append_character}= $type eq '%' ? "{" : "[";
                        last;
                     }
                  }
               }
            }
         }
         1
      },

      sub {
         my ($start_pos)=capturing_group_boundaries('args_start');
         $start_pos //= $-[0];
         my $varname=$+{varname};
         my $type= defined($+{key}) ? '%' : $+{type};
         my @proposals=grep { defined } $User::application->help->find("custom", qr{\Q$type$varname\E});
         ($start_pos, @proposals)
       }
   ),

   new Helper(
      'property name in a constructor call',

      qr{ $op_re new \s+ (?'type' $type_re) $meth_args_start_re
          (?: $expression_re ,\s* )* (?: (?'prop_name' $id_re) | $quote_re (?'chain' $hier_id_re \.?)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($type, $prop_name, $quote, $chain)=@+{qw(type prop_name quote chain)};

         if (defined ($type=$User::application->eval_type($type, 1))
               and
             instanceof ObjectType($type)) {
            $shell->completion_words=[ try_property_completion($type, $prop_name // $chain) ];
            if (defined $prop_name) {
               $_ .= "=>" for @{$shell->completion_words};
            } else {
               $shell->term->Attribs->{completion_append_character}=$quote;
            }
            return 1;
         }
         0
      },

      sub {
         my ($type, $prop_name, $chain)=@+{qw(type prop_name chain)};
         # next time tell about the object type
         my $start_pos=(capturing_group_boundaries('type'))[0]+1;
         my @proposals;

         if (defined ($type=$User::application->eval_type($type, 1))
               and
             instanceof ObjectType($type)) {
            @proposals=fetch_property_help($type, $prop_name // $chain);
         }
         ($start_pos, @proposals);
      }
   ),

   new Helper(
      'type expression',

      qr{ $op_re (?'static' (?:new|typeof|instanceof) \s+)?+
                 (?'open' (?: $qual_id_re \s*<\s* (?: $type_re \s*,\s*)* )+ )?
                 (?('static') | (?('open') | $ . ))   # if neither static nor open has matched, provoke the failure
                 (?'expr' $type_re (?(?<=\w)::)? )? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($static, $open, $expr)=@+{qw(static open expr)};
         if ($expr !~ /[<>]/) {
            # TODO: don't list the functions expecting ObjectType literally (how to detect ???)
            my $only_objects= (!$static && $open =~ /^(?: cast )\b/) || undef;
            my ($app, $pkg);
            if (defined ($app=try_type_completion($shell, $word, $expr, $only_objects)) &&
                @{$shell->completion_words}==1 &&
                defined ($pkg=namespaces::lookup_class($app->pkg, $shell->completion_words->[0]))) {
               if (my $min_params=eval { $pkg->_min_params }) {
                  # the suggested type has itself mandatory type parameters
                  $shell->term->Attribs->{completion_append_character}="<";
               } elsif (!defined($min_params)) {
                  # the suggested type is simple, the outer type list may be closed or continued
                  if ($shell->partial_input !~ m{\G \s*[,>]}xgc &&
                      (my @delims=type_expr_delimiter($open, $static)) == 1) {
                     $shell->term->Attribs->{completion_append_character}=$delims[0];
                  }
               }
            }
         } elsif ($shell->partial_input !~ m{\G \s*[,>]}xgc) {
            $shell->completion_words=[ type_expr_delimiter($open, $static) ];
         }
         1
      },

      sub {
         my $start_pos=$-[0];
         my $open_start=(capturing_group_boundaries('open'))[0];
         my ($open, $expr)=@+{qw(open expr)};
         my ($innermost_open, $innermost_open_start, $innermost_open_end)=
            $open =~ /($qual_id_re) \s*[<,]\s* $/xo ? ($1, $-[1], $+[1]) : ();
         my @proposals;

         if (defined $expr) {
            if ($expr =~ /^($qual_id_re) \s* (?: < | $ )/xo) {
               @proposals=fetch_type_help($1);
            }
            $start_pos=$innermost_open_end+$open_start if defined $innermost_open;
         } elsif (defined $innermost_open) {
            @proposals=fetch_type_help($innermost_open);
            $start_pos=$innermost_open_start+$open_start;
         }
         ($start_pos, @proposals);
      }
   ),

   new Helper(
      'symbolic color name',

      qr{(?: \$Visual::Color::$id_re \s*=\s* | Color \s*=>\s* ) (?: $quote_re (?!\d)(?'prefix' $non_quote_re*))? $}xo,

      sub {
         my ($shell, $word)=@_;
         my ($quote, $prefix)=@+{qw(quote prefix)};

         if (defined $quote) {
            finalize_proposals($shell, $prefix, $word, sort( Visual::list_color_completions($prefix) ));
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_words=[ '"' ];
         }
         1
      }
      # no context help
   ),

   new Helper(
      'function or method name or keyword argument',

      qr{ (?: (?'stmt' $statement_start_re) (?'func' $id_re)
            | $op_re (?: (?'method_owner' $var_or_func_re) $intermed_chain_re (?:(?'last_method_name' $id_re) $meth_args_start_re?+ )?
                       | $func_name_re (?'tparams' (?=\s*<) $confined_re)? $args_start_re?+ )
                     (?('args_start') (?'preceding_args' (?:$expression_re ,\s*)*+) (?'prefix' .*))
          ) $}xo,

      sub : method {
         my ($self, $shell, $word)=@_;
         my ($stmt, $var, $type, $app, $app_name, $func, $tparams, $intermed, $last_method_name, $args_start, $preceding_args, $prefix, @func_names);

         do {
            ($stmt, $var, $app_name, $func, $tparams, $intermed, $last_method_name, $args_start, $preceding_args, $prefix)=@+{qw(
              stmt   var   app_name   func   tparams   intermed   last_method_name   args_start   preceding_args   prefix)   };

            my @proposals;
            if (defined $+{method_owner}) {
               # a method
               if (($var, $type)=retrieve_method_owner_type($var, $app_name, $func, $intermed)) {
                  if (defined $args_start) {
                     if (instanceof ObjectType($type)) {
                        if (defined (my $prop=$type->lookup_property($last_method_name))) {
                           if ($prop->flags & $Property::is_multiple) {
                              if (length($preceding_args)==0  and
                                  $prefix =~ s/^($anon_quote_re)(?=$non_quote_re+$)//) {
                                 # name of a subobject
                                 if (defined($var)) {
                                    $shell->term->Attribs->{completion_append_character}=$1;
                                    $shell->completion_words=[ sorted_uniq(sort( grep { defined($_) && index($_, $prefix)==0 }
                                                                                 map { $_->name } @{$var->give($last_method_name)} )) ];
                                 }
                                 return 1;
                              }
                              if ($prefix =~ /^($id_re)?$/o) {
                                 # property for selecting a subobject
                                 @proposals=map { "$_=>" } try_property_completion($prop->specialization($type)->type, $prefix);
                              }
                           }
                           if (index("temporary", $prefix)==0) {
                              push @proposals, "temporary";
                           }
                        } elsif (length($preceding_args)>0 &&
                                 $last_method_name =~ /^(?:give|take|add)$/ &&
                                 index("temporary", $prefix)==0) {
                           push @proposals, "temporary";
                        }
                        if (@proposals) {
                           # prefix is simple, no need to reiterate
                           $shell->completion_words=\@proposals;
                           return 1;
                        }
                     } elsif (try_keyword_completion($shell, $preceding_args, $prefix, $word,
                                                     uniq(retrieve_method_topics($type, $last_method_name, "find")))) {
                        return 1;
                     }
                  } else {
                     # completing the method name
                     @proposals=retrieve_method_topics($type, $last_method_name, "list_completions");
                     if (ref($type)) {
                        if (instanceof ObjectType($type)) {
                           @proposals=grep { !$type->is_overridden($_) } @proposals;
                           # TODO: describe all common methods in help.rules and discard %object_methods
                           if (my @common_methods=grep { index($_, $last_method_name)==0 } keys %object_methods) {
                              if (@common_methods==1 && !@proposals &&
                                  $common_methods[0] !~ /^(?:name|description|list_attachments)$/) {
                                 $shell->term->Attribs->{completion_append_character}="(";
                              }
                              push @proposals, @common_methods;
                           }
                        } elsif ($type->cppoptions && $type->cppoptions->fields) {
                           push @proposals, grep { index($_, $last_method_name)==0 } @{$type->cppoptions->fields};
                        }
                     }
                     if (@proposals) {
                        @func_names=sorted_uniq(sort(@proposals));
                     }
                  }
               }
            } else {
               # a free function
               if (defined ($app= defined($app_name) ? eval { User::application($app_name) } : $User::application)) {
                  if (defined $args_start) {
                     if (try_keyword_completion($shell, $preceding_args, $prefix, $word,
                                                $app->help->find((defined($app_name) ? "!rel" : ()), "functions", $func))) {
                        return 1;
                     }
                  } elsif (defined $tparams) {
                     # complete name and type param list, missing arguments
                     $shell->completion_words=[ "(" ];
                     return 1;
                  } else {
                     # completing the function name
                     if (defined $app_name) {
                        @proposals=map { "$app_name\::$_" } $app->help->list_completions("!rel", "functions", $func);
                     } else {
                        @proposals=$app->help->list_completions("functions", $func);
                        if (defined $stmt) {
                           push @proposals, grep { index($_, $func)==0 } keys %popular_perl_keywords;
                        }
                     }
                     if (@proposals) {
                        @func_names=sorted_uniq(sort(@proposals));
                        if (@func_names==1) {
                           $func= defined($app_name) ? substr($func_names[0], length($app_name)+2) : $func_names[0];
                           if ($func =~ /^(?:exit|history|replay_history|show_(?:preferences|unconfigured|credits|extensions))$/) {
                              $shell->term->Attribs->{completion_append_character}=";";
                           } elsif (exists $popular_perl_keywords{$func} ||
                                    $func =~ /^(?:application|help|apropos|include|prefer(?:_now)?|(?:re)?set_(?:custom|preference)|(?:re|un)configure)$/) {
                              $shell->term->Attribs->{completion_append_character}=" ";
                           } elsif (defined (my $h=$app->help->find("functions", $func))) {
                              if (my ($min, $max)=$h->expects_template_params) {
                                 if ($min>0) {
                                    $shell->term->Attribs->{completion_append_character}="<";
                                 }
                              } else {
                                 $shell->term->Attribs->{completion_append_character}="(";
                              }
                           }
                        }
                     }
                  }
               }
            }
            # descend to the innermost open function call
         } while (defined($args_start) && $prefix =~ /${$self->pattern}/);

         $shell->completion_words=\@func_names;
         @func_names>0
      },

      sub : method {
         my ($self, $shell)=@_;
         my (@result, $var, $type, $app, $app_name, $func, $intermed, $last_method_name, $args_start, $prefix);
         my $adjust_pos=0;
         do {
            ($var, $app_name, $func, $intermed, $last_method_name, $args_start, $prefix)=@+{qw(
              var   app_name   func   intermed   last_method_name   args_start   prefix)   };
            my $start_pos=$-[0]+$adjust_pos;
            my $method_name_pos=(capturing_group_boundaries('last_method_name'))[0]+$adjust_pos;
            $adjust_pos += (capturing_group_boundaries('prefix'))[0];
            my @proposals;

            if (defined $last_method_name) {
               # a method
               if (($var, $type)=retrieve_method_owner_type($var, $app_name, $func, $intermed)) {
                  if (defined($args_start) && $prefix =~ /^$id_re$/o) {
                     if (instanceof ObjectType($type)
                           and
                         defined (my $prop=$type->lookup_property($last_method_name))) {
                        # property for selecting a subobject
                        if ($prop->flags & $Property::is_multiple
                              and
                            @proposals=fetch_property_help($prop->specialization($type)->type, $prefix)) {
                           # next time match the multiple property
                           return ($method_name_pos+1, @proposals);
                        }
                     }
                  }
                  if (@proposals=retrieve_method_topics($type, $last_method_name, "find")) {
                     # next time try matching enclosing function
                     @result=($start_pos, uniq(@proposals));
                  }
               }
            } else {
               # a free function
               if (defined (my $app= defined($app_name) ? eval { User::application($app_name) } : $User::application)) {
                  if (@proposals=$app->help->find("functions", $func)) {
                     # next time try matching enclosing function
                     @result=($start_pos, uniq(@proposals));
                  }
               }
            }

            # descend to the innermost open function call
         } while ($prefix =~ /${$self->pattern}/gc);
         @result
      }
   ),
);

sub matching_rulefiles {
   my ($app, $cmd, $prefix)=@_;
   if ($cmd eq "include") {
      map { $_ =~ $filename_re } map { glob("$_/$prefix*") } @{$app->rulepath}
   } else {
      grep { /^\Q$prefix\E/ } $app->list_configured($cmd eq "unconfigure")
   }
}

sub try_label_completion {
   my ($app, $expr)=@_;
   sorted_uniq(sort( map { $_->list_completions($expr) } $app->prefs, @{$app->prefs->imported} ))
}

sub prepare_property_lookup {
   my ($type, $prefix, $allow_auto_cast)=@_;
   my @path=split /\./, $prefix;
   $prefix= substr($prefix,-1,1) eq "." ? "" : pop @path;
   foreach (@path) {
      my $prop=$type->lookup_property($_) || ($allow_auto_cast && $type->lookup_auto_cast($_)) or return;
      instanceof ObjectType($type=$prop->specialization($type)->type) or return;
   }
   ($type, $prefix);
}

sub try_property_completion {
   my ($type, $prefix, $allow_auto_cast)=@_;
   if (($type, $prefix)=&prepare_property_lookup) {
      sorted_uniq(sort( grep { !$type->is_overridden($_) } grep { /^$prefix/ } map { keys %{$_->properties} }
                        map { ($_, $allow_auto_cast ? keys %{$_->auto_casts} : ()) } $type, @{$type->super} ))
   } else {
      ()
   }
}

sub fetch_property_help {
   my ($type, $prefix, $allow_auto_cast)=@_;
   $prefix =~ s/\.$//;
   if (($type, $prefix)=&prepare_property_lookup) {
      my $topic;
      uniq( grep { !$type->is_overridden($_->name) }
                 map { defined($topic=$_->help_topic) ? $topic->find("!rel", "properties", $prefix) : () }
                     map { ($_, $allow_auto_cast ? keys %{$_->auto_casts} : ()) } $type, @{$type->super} )
   } else {
      ()
   }
}

sub prepare_type_matching {
   my ($prefix, $is_object_type)=@_;
   my $maybe_object_type= !( $prefix =~ s/\bprops::// ) && (!defined($is_object_type) || $is_object_type);
   my $maybe_prop_type= !( $prefix =~ s/\bobjects::// ) && !$is_object_type;
   my $app_name= $prefix =~ s/^($id_re)::// && $1;
   ($prefix, $app_name, $maybe_object_type, $maybe_prop_type)
}

sub try_type_completion {
   my ($shell, $word, $prefix, $is_object_type)=@_;
   my ($type_prefix, $app_name, @maybe)=prepare_type_matching($prefix, $is_object_type);
   my $app= $app_name ? (eval { User::application($app_name) } || return) : $User::application;
   my @how= $app_name ? ("!rel") : ();
   my @proposals;
   foreach my $whence (qw(objects property_types)) {
      if (shift @maybe) {
         push @proposals, $app->help->list_completions(@how, $whence, $type_prefix);
      }
   }

   if (@proposals) {
      finalize_proposals($shell, $type_prefix, $word, sorted_uniq(sort(@proposals)));
      $app
   } else {
      if ($prefix !~ /::/) {
         $shell->completion_words=[ map { "$_\::" } sort( grep { /^$prefix/ } (($is_object_type ? "objects" : qw(objects props)), keys %{$app->used}) ) ];
      }
      undef
   }
}

sub type_expr_delimiter {
   my ($open, $static)=@_;
   my @proposals;
   if (length($open)) {
      my $cnt=1;
      ++$cnt while $open =~ s/$type_re \s*,\s* $//xo;
      $open =~ s/($qual_id_re) \s*<\s* $//xo;
      my $outer=$1;
      my ($min_params, $max_params);
      if (!$static && length($open)==0) {
         # a parametrized function
         my $app= $outer =~ s/^($id_re)::// ? (eval { User::application($1) } || return) : $User::application;
         if (defined (my $h=$app->help->find("functions", $outer))) {
            ($min_params, $max_params)=$h->expects_template_params;
         }
      } else {
         # a parametrized type
         if (defined (my $pkg=namespaces::lookup_class($User::application->pkg, $outer))) {
            ($min_params, $max_params)=eval { ($pkg->_min_params, scalar(@{$pkg->typeof_gen->params})) };
         }
      }
      if (defined($max_params) && $cnt < $max_params) {
         push @proposals, ",";
      }
      if (defined($min_params) && $cnt >= $min_params) {
         push @proposals, ">";
      }
   }
   @proposals
}

sub fetch_type_help {
   my ($prefix, $is_object_type)=@_;
   my ($type_name, $app_name, @maybe)=prepare_type_matching($prefix, $is_object_type);
   my @proposals;
   if (defined (my $app= $app_name ? eval { User::application($app_name) } : $User::application)) {
      my @how=$app_name ? ("!rel") : ();
      foreach my $whence (qw(objects property_types)) {
         if (shift @maybe) {
            push @proposals, $app->help->find(@how, $whence, $type_name);
         }
      }
   }
   uniq(@proposals);
}

sub retrieve_return_type {
   my $name=$_[0]->return_type;
   if (defined($name) and $name !~ /::/) {
      $User::application->eval_type($name, 1)
   } else {
      $name
   }
}

sub retrieve_method_owner_type {
   my ($var, $app_name, $func, $intermed)=@_;
   my ($app, $type, $topic, @how);

   if (defined($var)) {
      package Polymake::User;
      $type=eval "$var->type"
        and
      $var= length($intermed) ? undef : eval $var;
   } else {
      if (defined $app_name) {
         $app=eval { User::application($app_name) } or return;
         @how=qw(!rel);
      } else {
         $app=$User::application;
      }
      if (defined ($func=$app->help->find(@how, "functions", $func))) {
         if (!defined ($type=retrieve_return_type($func)) &&
             $func->parent->category &&
             $func->parent->name =~ /^\s* (?: producing | transforming ) /xi) {
            $type=$app->default_type;
         }
      }
   }

   if (defined($type)) {
      my $prop;
      while ($intermed =~ m{\G $intermed_re }gxo) {
         if (defined (my $method_name=$+{method_name})) {
            undef $prop;
            if (instanceof ObjectType($type) and defined ($prop=$type->lookup_property($method_name, 1))) {
               $type=$prop->specialization($type)->type;
            } elsif (defined ($func= ref($type) ? ($topic=$type->help_topic and $topic->find("methods", $method_name))
                                                : ($topic=$User::application->help->find("objects", $type)) and $topic->find("methods", $method_name))) {
               if (defined (my $ret_type=retrieve_return_type($func))) {
                  $type=$ret_type;
               }
               # otherwise let's just suppose the method returns the same object
            } else {
               ref($type) or defined ($type=$User::application->eval_type($type, 1)) or return;
               if (instanceof PropertyType($type)) {
                  $type=$type->get_field_type($method_name) or return;
               } else {
                  return;
               }
            }
         } elsif (not defined($prop) && $prop->flags & $Property::is_multiple) {
            # a bracketed index expression
            $type=$type->get_element_type or return;
         }
      }
   }

   ($var, $type)
}

sub retrieve_method_topics {
   my ($type, $name, $help_method)=@_;
   if (ref($type)) {
      if (instanceof ObjectType($type)) {
         return map { $_->$help_method("!rel", "properties", "methods", $name) }
                    ( grep { defined } map { $_->help_topic }
                      ($type, @{$type->super}, keys %{$type->auto_casts}) ),
                    $type->application->help->find(qw(objects Core::Object));
      }

      my @proposals;
      do {
         if (defined (my $topic=$type->help_topic)) {
            push @proposals, $topic->$help_method("!rel", "methods", $name);
         }
         $type=$type->super;
      } while (defined $type);
      return @proposals;
   }

   no strict 'refs';
   map {
      map { $_->$help_method("!rel", "methods", $name) } $User::application->help->find("objects", $_)
   } $type, @{"$type\::ISA"};
}

# completing a keyword or enumerated argument
sub try_keyword_completion {
   my ($shell, $preceding_args, $prefix, $word, @topics)=@_;
   my $arg_num=0;
   ++$arg_num while $preceding_args =~ /\G $expression_re ,\s* /xog;
   if (my @proposals=map { $_->argument_completions($arg_num, $prefix) } @topics) {
      finalize_proposals($shell, $prefix, $word, sorted_uniq(sort(@proposals)));
      1
   }
}

sub complete_variable_name_in_pkg {
   my ($pkg, $type, $prefix)=@_;
   my @proposals;
   while (my ($name, $glob)=each %$pkg) {
      if (length($prefix) ? index($name,$prefix)==0 : $name !~ /^\./) {
         if (defined_scalar($glob) && $type eq '$' or
             defined(*{$glob}{ARRAY}) && ($type eq '@' || $type eq '$') or
             defined(*{$glob}{HASH}) && ($type eq '%' || $type eq '$' || $name =~ /::$/)) {
            push @proposals, $name;
         }
      }
   }
   @proposals
}

sub complete_user_variable_name {
   my ($type, $prefix)=@_;
   if ((my $pkg_end=rindex($prefix,"::"))>0) {
      my $pkg_part=substr($prefix, 0, $pkg_end);
      my $pkg=get_pkg("Polymake::User::$pkg_part") or return ();
      map { "$pkg_part\::$_" } complete_variable_name_in_pkg($pkg, $type, substr($prefix, $pkg_end+2));
   } else {
      complete_variable_name_in_pkg(\%Polymake::User::,@_);
   }
}

sub find_user_variable {
   my ($type, $name)=@_;
   my $glob=do {
      if ((my $pkg_end=rindex($name, "::"))>0) {
         my $pkg=get_pkg("Polymake::User::" . substr($name,0,$pkg_end)) or return;
         $pkg->{substr($name,$pkg_end+2)};
      } else {
         $Polymake::User::{$name};
      }
   } or return;

   $type eq '%'
   ? *{$glob}{HASH} :
   $type eq '@'
   ? *{$glob}{ARRAY} :
   $type eq '$'
   ? *{$glob}{SCALAR}
   : $glob
}

# path or URI, allow_bundled => Extension or undef
sub verify_extension {
   my ($ext_dir, $allow_bundled)=@_;
   replace_special_paths($ext_dir);
   my $ext;
   if (($ext_dir !~ m{^/} && defined ($ext=$Extension::registered_by_URI{$ext_dir}))
       || defined ($ext=$Extension::registered_by_dir{$ext_dir})
         and
       $allow_bundled || !$ext->is_bundled) {
      $ext
   } else {
      undef
   }
}

# path or URI => path or undef
sub verify_ext_dir {
   if (defined (my $ext=&verify_extension)) {
      $ext->dir;
   } else {
      undef
   }
}

# readline expects proposals for the trailing word only, without any separators and interpuctuation;
# if the end of the input line matches a more complex syntactic construction, the proposals must be trimmed
sub finalize_proposals {
   my ($shell, $prefix, $word, @proposals)=@_;
   my $trim=length($prefix)-length($word);
   if ($trim>0) {
      $shell->completion_words=[ map { substr($_, $trim) } @proposals ];
   } elsif ($trim<0) {
      my $prepend=-$trim;
      $word=substr($word, 0, $prepend);
      $shell->completion_words=[ map { substr($_, 0, $prepend) eq $word ? $_ : $word.$_ } @proposals ];
   } else {
      $shell->completion_words=\@proposals;
   }
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
