#  Copyright (c) 1997-2021
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
use feature 'state';
use namespaces;
use warnings qw(FATAL void syntax misc);

# Analyzers of partial input, supporting TAB completion and F1 context-sensitive help

package Polymake::Core::Shell::Completion;

use Polymake::Struct (
   [ new => '$$$;$' ],
   [ '$name' => '#1' ],         # descriptive name
   [ '$pattern' => '#2' ],      # pattern to match the partial input
   [ '&completion' => '#3' ],   # ($Shell) => 1 for `ready' || 0 or undef for `try next pattern'
   [ '&help' => '#4' ],         # ($Shell) => (start position in partial input, Help::Topic ...) || () for `try next pattern'
);

# the term is NOT a method call, if preceded by:
my $op_re = qr{(?:[-+*/.,<=!~;(\{\}\[]| (?<!-)> | ^ )\s* | \w\s+ }x;

# a variable or array element
my $var_re = qr{ \$ $id_re (?:(?=\s*\[) $confined_re)? }xo;

# a function name, possibly qualified
my $func_name_re = qr{(?:(?'app_name' $id_re):: | (?! (?:if|unless|while|until|for(?:each)?|or|and|not|print)[\s(])) (?'func' $id_re)}xo;

# start of a method call chain: a variable or a function call
my $var_or_func_re = qr{ (?'var' $var_re) | $func_name_re (?(?=\s*\() $confined_re)? }xo;

# intermediate expression in a method call chain
my $intermed_re = qr{ \s*->\s* (?: (?'method_name' $id_re) (?(?=\s*\() \s* (?'call_args' $confined_re))?+ | (?=\[) $confined_re ) }xo;

# chain of intermediate expressions
my $intermed_chain_re = qr{ (?'intermed' (?:$intermed_re)*?) \s*->\s* }xo;

# a keyword, bare or quoted, with an arrow
my $anon_keyword_re = qr{ (?: $id_re | ($anon_quote_re) $non_quote_space_re+ \g{-1}) \s*=>\s* }xo;

# same, with named capturing group
my $keyword_re = qr{(?:(?'keyword' $id_re) | ($anon_quote_re) (?'keyword' $non_quote_space_re+) \g{-2}) \s*=>\s* }xo;

# incomplete argument list during completion
my $arg_list_re = qr{(?'preceding_args' (?:$expression_re ,\s*)*+)
                     (?'grouped_with' $anon_keyword_re? [\[\{]\s* (?: $anon_keyword_re $expression_re ,\s*)*+)?+
                     (?: $keyword_re? (?: $quote_re (?'text' $non_quote_re*) | (?'text' \w*))) }xo;

################################################################################################
#
#  useful data for completers
#
package Polymake::Core::Shell::Completion::Context;

use Polymake::Struct (
   [ new => '$$$$$;$' ],
   [ '$shell' => '#1' ],             # Core::Shell instance
   [ '$quote' => '#2' ],             # leading quote, if any
   [ '$keyword' => '#3' ],           # keyword in a function call the value being completed is assigned to
   [ '$obj' => '#6' ],               # object or its type when a method argument is being completed
   '@preceding_args',                # preceding function arguments as input strings
   [ '$grouped_with' => 'undef' ],   # preceding keywords and value expressions in the unfinished group (hash or array) as input strings
   [ '$group_keyword' => 'undef' ],  # keyword in a function call the unfinished group is assigned to
   [ '$tag' => 'undef' ],            # completer tag as specified in doc strings
   [ '$values' => 'undef' ],         # value list as specified in doc strings
   [ '$prop_name' => 'undef' ],      # property name used as a method name
);

sub new {
   my ($preceding_args, $grouped_with) = @_[4, 5];
   my $self = &_new;
   while ($preceding_args =~ /\G ($expression_re) ,\s* /gxo) {
      push @{$self->preceding_args}, $1;
   }
   if ($grouped_with =~ s/^$keyword_re? \{//x) {
      $self->group_keyword = $+{keyword};
      my %preceding_keywords;
      while ($grouped_with =~ /\G\s* $keyword_re (?'value' $expression_re) ,/gxo) {
         $preceding_keywords{$+{keyword}} = $+{value};
      }
      $self->grouped_with = \%preceding_keywords;
   } elsif ($grouped_with =~ s/^$keyword_re? \[//x) {
      $self->group_keyword = $+{keyword};
      my @preceding_keywords;
      while ($grouped_with =~ /\G\s* $keyword_re (?'value' $expression_re) ,/gxo) {
         push @preceding_keywords, $+{keyword}, $+{value};
      }
      $self->grouped_with = \@preceding_keywords;
   }
   $self;
}

sub is_real_object {
   my $obj = $_[0]->obj;
   is_object($obj) && !instanceof PropertyType($obj) && !instanceof BigObjectType($obj)
}

sub extract_simple_arg {
   my ($self, $index) = @_;
   my $arg = $self->preceding_args->[$index];
   if ($arg =~ s/^$quoted_re$/$+{quoted}/o) {
      $arg
   } elsif ($arg =~ /^\$$qual_id_re$/) {
      package Polymake::User;
      eval $arg
   } else {
      undef
   }
}

################################################################################################
#
#  Argument-specific completers, selected by [complete NAME] hints in doc strings
#
package Polymake::Core::Shell::Completion::Param;

use Polymake::Enum Flags => { quoted => 1, object => 2 };

use Polymake::Struct (
   [ new => '$%' ],
   [ '&completion' => '#1' ],
   [ '&help' => '#%', default =>'undef' ],
   [ '$pattern' => '#%', default => 'undef' ],
   [ '$flags' => '#%', default => '0' ],
);

my %completers = (
   # file and directory name completion
   file => new Param(
      sub {
         my ($text, $ctx) = @_;
         try_filename_completion($ctx->shell, $text, "file")
      },
      flags => Flags::quoted
   ),

   dir => new Param(
      sub {
         my ($text, $ctx) = @_;
         try_filename_completion($ctx->shell, $text, "dir")
      },
      flags => Flags::quoted
   ),

   # values declared in the doc strings
   value_list => new Param(
      sub {
         my ($text, $ctx) = @_;
         grep { /^\Q$text\E/ } map { $_->value =~ $quoted_re ? $+{quoted} : () } @{$ctx->values}
      },
      flags => Flags::quoted
   ),

   # application name in commands like application, extend_application
   'Core::Application' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my @apps = map { $_ =~ $filename_re } map { glob "$_/apps/$text*" }
                        $InstallTop, map { $_->dir } @Extension::active[$Extension::num_bundled .. $#Extension::active];
         if (@apps && defined(my $ext_dir = $ctx->preceding_args->[0])) {
            $ext_dir = eval($ext_dir) or return;
            replace_special_paths($ext_dir);
            if ($ext_dir !~ m{^/} && defined(my $ext = $Extension::registered_by_URI{$ext_dir})) {
               $ext_dir = $ext->dir;
            }
            grep { !-d "$ext_dir/apps/$_" } @apps
         } else {
            @apps
         }
      },
      help => sub {
         my ($text) = @_;
         if (defined(my $app = eval { User::application($text) })) {
            $app->help
         } else {
            ()
         }
      },
      pattern => qr/^\w*$/,
      flags => Flags::quoted
   ),

   # property name
   'Core::Property' => new Param(
      do {
         my %patterns = ( give => qr/^(?: $prop_path_re \s*\|\s*)* (?'prop' $prop_path_re\.?)? $/xo,
                          take => qr/^(?'prop' $prop_path_re\.?)? $/xo,
                          add => qr/^(?'prop' $prop_name_re)? $/xo );
         ( sub {
              my ($text, $ctx) = @_;
              $text =~ $patterns{$ctx->tag} or return;
              my $prop_name = $+{prop};
              my @proposals = try_property_completion($ctx->obj->type, $prop_name, $ctx->tag eq "add");
              $ctx->shell->completion_offset = length($prop_name);
              @proposals
           },
           help => sub {
              my ($text, $ctx) = @_;
              $text =~ $patterns{$ctx->tag} or return;
              fetch_property_help_or_warn($ctx->obj->type, $+{prop})
           })
      }, flags => Flags::quoted
   ),

   # property name for a multiple subobject
   'Core::Property::InMulti' => new Param(
      sub {
         my ($text, $ctx) = @_;
         if ($ctx->tag eq "add" && @{$ctx->preceding_args} == 1 && !$ctx->quote && index("temporary", $text) == 0) {
            $ctx->shell->completion_append_character = ',';
            return "temporary";
         }
         defined(my $multi_prop_name = $ctx->prop_name // $ctx->extract_simple_arg(0)) or return;
         my @proposals;
         if ($ctx->tag ne "add" && $ctx->is_real_object && $ctx->quote && @{$ctx->preceding_args} == !defined($ctx->prop_name)) {
            # name of a subobject instance?
            if (defined(my $pv = $ctx->obj->lookup_pv($multi_prop_name))) {
               @proposals = grep { defined($_) && index($_, $text) == 0 }
                            map { $_->name } @{$pv->values};
            }
            if (@proposals && length($text) == 0) {
               # if there was a naked trailing quote, assume the user wanted to search by subobject names only,
               # since the list of all property names can be requested by pressing TAB after ( or ,
               return @proposals;
            }
         }
         my $prop_name = "$multi_prop_name.$text";
         if (my @subprop_proposals = try_property_completion($ctx->obj->type, $prop_name)) {
            $ctx->shell->completion_offset = length($prop_name);
            if ($ctx->quote) {
               push @proposals, @subprop_proposals;
            } else {
               push @proposals, map { "$_=>" } @subprop_proposals;
            }
         }
         @proposals
      },
      help => sub {
         my ($text, $ctx) = @_;
         $ctx->tag eq "add" && @{$ctx->preceding_args} == 1 && !$ctx->quote && $text eq "temporary" and return;
         defined(my $multi_prop_name = $ctx->prop_name // $ctx->extract_simple_arg(0)) or return;
         if ($ctx->tag ne "add" && $ctx->quote && @{$ctx->preceding_args} == !defined($ctx->prop_name)) {
            # maybe it's a name of a subobject instance, then silently move to the method itself
            fetch_property_help($ctx->obj->type, "$multi_prop_name.$text")
         } else {
            fetch_property_help_or_warn($ctx->obj->type, "$multi_prop_name.$text")
         }
      }
   ),

   # property name in a JSON schema
   'Core::Property::InSchema' => new Param(
      sub {
         my ($text, $ctx) = @_;
         if ($ctx->keyword) {
            # no proposals for values implemented yet
            ()
         } else {
            # do not propose meta-properties
            $text =~ s/(?:^|\.)\K$/[a-zA-Z]/;
            $ctx->obj->list_property_completions($text)
         }
      }
   ),

   # script file name
   scriptfile => new Param(
      sub {
         my ($text, $ctx) = @_;
         my @locations;
         if ($text !~ m{^[./~]}) {
            @locations = ((map { @{$_->scriptpath} } $User::application, @{$User::application->linear_imported}),
                          @User::lookup_scripts, "$InstallTop/scripts");
         }
         try_filename_completion($ctx->shell, $text, "file", @locations)
     },
     flags => Flags::quoted
   ),

   # symbolic color name in an option
   color => new Param(
      sub {
         my ($text) = @_;
         if ($text !~ /^\d/) {
            sort( Visual::list_color_completions($text) )
         } else {
            ()
         }
      },
      flags => Flags::quoted
   ),

   # help topic
   'Core::Help' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my $split_path = qr{^(.*/)([^/]*)$};
         my ($top_help, $prepend_slash, @how, $path, $leaf);
         if ($text =~ m{^($id_re)(?:(::)|(?=$|/))}o  &&  defined(my $app = lookup Application($1))) {
            $top_help = $app->help;
            push @how, "!rel";
            $text = $';
            my $colons = $2;
            unless (($path, $leaf) = $text =~ $split_path) {
               if ($colons) {
                  $leaf = $text;
                  push @how, "functions", "objects", "property_types";
               } else {
                  $path = $leaf = "";
                  $prepend_slash = true;
               }
            }
         } elsif ($text =~ m{^core(?=/)}) {
            $top_help = $Help::core;
            $text = $';
            ($path, $leaf) = $text =~ $split_path;
         } else {
            $top_help = $User::application->help;
            unless (($path, $leaf) = $text =~ $split_path) {
               $leaf = $text;
            }
         }
         my @proposals = sorted_uniq(sort( map { $_->name }
                            (defined($path)
                             ? ( map { $_->list_matching_children($leaf) } $top_help->get_topics($path) )
                             : $top_help->list_matching_topics(@how, $leaf) )));
         if ($prepend_slash) {
            substr($_, 0, 0) .= "/" for @proposals;
         }
         if (@proposals == 1 &&
             grep { @{$_->toc} } $top_help->get_topics($path.$proposals[0])) {
            $ctx->shell->completion_append_character = "/";
         } elsif (!@how && !defined($path)) {
            undef $ctx->shell->completion_append_character if !@proposals;
            push @proposals, grep { /^\Q$leaf\E/ } "core", map { $_->name } list_loaded Application;
         }
         $ctx->shell->completion_offset = length($leaf);
         @proposals
      },
      flags => Flags::quoted
   ),

   # label argument for preference manipulations
   'Core::Preference' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my ($app_name, $expr) = @+{qw(app_name expr)};
         my $app = defined($app_name) ? (eval { User::application($app_name) } // return ()) : $User::application;
         my @proposals = map { $_->list_completions($expr) } $app->prefs, @{$app->prefs->imported};
         if ($ctx->tag ne "reset" && $expr =~ /^\*\.$hier_id_re\s*$/o) {
            $ctx->shell->completion_append_character = " ";
         } elsif (!defined($app_name) && $expr =~ /^\w*$/o) {
            push @proposals, matching_app_prefixes($ctx->shell, $app, $expr);
         }
         $ctx->shell->completion_offset = $expr =~ /\b\w+$/o ? length($&) : 0;
         @proposals
      },
      help => sub {
         my ($text) = @_;
         my ($app_name, $expr) = @+{qw(app_name expr)};
         my $app = defined($app_name) ? (eval { User::application($app_name) } // return ()) : $User::application;
         if ($expr =~ /($id_re) (?: \. $id_re)* \.? \s* $/xo) {
            $app->help->find('preferences', $1);
         } else {
            ()
         }
      },
      pattern => qr{^ (?: (?'app_name' $id_re) ::)?
                      (?'expr' \*\.(?: $hier_id_re (?: \s+ (?: $ids_re (?:\s*,)? )? | \.? )? )? | $hier_id_re\.?)? $}xo,
      flags => Flags::quoted
   ),

   # rulefile, possible qualified with application prefix and extension URI
   'Core::Rulefile' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my @proposals;
         if ($text =~ s/^($id_re):://) {
            my $app_name = $1;
            if (defined(my $app = lookup Application($app_name))) {
               @proposals = matching_rulefiles($app, $ctx->tag, $text);
            }
            $ctx->shell->completion_offset = length($text);
         } else {
            @proposals = matching_rulefiles($User::application, $ctx->tag, $text);
            foreach my $app (values %{$User::application->used}) {
               if ($app->name =~ /^\Q$text\E/  and  matching_rulefiles($app, $ctx->tag, "")) {
                  push @proposals, $app->name."::";
                  undef $ctx->shell->completion_append_character;
               }
            }
         }
         @proposals
      },
      flags => Flags::quoted
   ),

   # rule label or header
   'Core::Rule' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my @proposals;
         if ($text =~ /^ (?: $hier_id_re (?:\.)? )? $/xo) {
            # maybe a label
            @proposals = grep { /^\Q$text\E/ } map { $_->full_name } map { @{$_->labels} } try_rules($ctx->obj);
         }
         Rule::prepare_header_search_pattern($text);
         push @proposals, grep { /^$text/ } map { $_->header } try_rules($ctx->obj);
         @proposals
      },
      help => sub {
         my ($text, $ctx) = @_;
         if ($text =~ /($id_re) (?: \. $id_re)* \.? \s* $/xo) {
            my $label_name = $1;
            (defined($ctx->obj) ? $ctx->obj->type->application : $User::application)->help->find('preferences', $label_name)
         } else {
            ()
         }
      },
      flags => Flags::quoted
   ),

   # attachment name
   attachment => new Param(
      sub {
         my ($text, $ctx) = @_;
         grep { /^\Q$text\E/ } keys %{$ctx->obj->attachments}
      },
      flags => Flags::quoted + Flags::object
   ),

   # extension directory or URI
   'Core::Extension' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my $match = $text =~ m{^/} ? "dir" : "URI";
         my @proposals = map {
            my $item = $_->$match;
            ($ctx->tag eq "bundled" || !$_->is_bundled) && $item =~ /^\Q$text\E/ ? ($item) : ()
         } values %Extension::registered_by_dir;

         if ($ctx->tag eq "core" && index("core", $text) == 0) {
            push @proposals, "core";
         }
         @proposals
      },
      help => sub {
         my ($text) = @_;
         my $ext = verify_extension($text, $ctx->tag);
         if (defined($ext) && defined($ext->credit) && length($ext->credit->text) > 0) {
            sub {
               print "Extension contains package ", $ext->credit->product, ":\n", $ext->credit->text;
            }
         } else {
            ()
         }
      },
      flags => Flags::quoted
   ),

   # extension configuration option
   'Core::Extension::Config' => new Param(
      sub {
         my ($text, $ctx) = @_;
         my $ext_dir = $ctx->preceding_args->[0];
         if (my $script = extension_configure_script($ext_dir, $ctx->tag)) {
            my (%allowed_options, %allowed_with, $err);
            {  package Polymake::StandaloneExt;
               do $script;
               $err = $@;
            }
            unless ($err) {
               eval "Polymake::StandaloneExt::allowed_options(\\%allowed_options, \\%allowed_with)";
               $err = $@;
            }
            wipe_package(\%Polymake::StandaloneExt::);
            unless ($err) {
               $ctx->shell->completion_append_character = '=';
               return grep { /^\Q$text\E/ }
                        (map { "--$_" } keys %allowed_options),
                        (map { ("--with-$_", "--without-$_") } keys %allowed_with),
                        qw(--defaults);
            }
         }
         ()
      },
      help => sub {
         my ($text, $ctx) = @_;
         my $ext_dir = $ctx->preceding_args->[0];
         if (my $script = extension_configure_script($ext_dir, $ctx->tag)) {
            sub {
               package Polymake::StandaloneExt;
               do $script;
               unless ($@) {
                  print STDERR "\nFollowing configuration options are possible:\n";
                  eval "Polymake::StandaloneExt::usage()";
               }
               wipe_package(\%Polymake::StandaloneExt::);
            }
         } else {
            ()
         }
      },
      pattern => qr{^ $non_quote_space_re* $}xo,
      flags => Flags::quoted
   ),
);

sub find {
   $completers{$_[1]}
}

################################################################################################
#
#  Helper routines for parameter completers
#

sub try_rules {
   my ($obj) = @_;
   if (defined($obj)) {
      my $type = $obj->type;
      map { @{$_->production_rules} } $type, @{$type->linear_isa}
   } else {
      map { @{$_->rules} } $User::application, values %{$User::application->used}
   }
}

sub matching_rulefiles {
   my ($app, $tag, $text) = @_;
   if ($tag eq "filename") {
      map { $_ =~ $filename_re } map { glob("$_/rules/$text*") } $app->top, map { $_->app_dir($app) } @{$app->extensions}
   } else {
      grep { /^\Q$text\E/ } $app->list_configured($tag eq "configured")
   }
}

# path or URI, allow_bundled => Extension or undef
sub verify_extension {
   my ($ext_dir, $tag) = @_;
   replace_special_paths($ext_dir);
   my $ext;
   if (($ext_dir !~ m{^/} && defined($ext = $Extension::registered_by_URI{$ext_dir}))
       || defined($ext = $Extension::registered_by_dir{$ext_dir})
         and
       $tag eq "bundled" || !$ext->is_bundled) {
      $ext
   } else {
      undef
   }
}

# path or URI => path or undef
sub extension_configure_script {
   my ($ext_dir, $tag) = @_;
   if (length($ext_dir)  and
       defined($ext_dir = eval $ext_dir)) {
      if ($tag eq "dir") {
         replace_special_paths($ext_dir);
      } elsif (defined(my $ext = verify_extension($ext_dir))) {
         $ext_dir = $ext->dir;
      } else {
         return;
      }
      my $script = "$ext_dir/support/configure.pl";
      -f $script and $script
   }
}

################################################################################################
#
#  Completers for the entire command line
#
#  The order is important: more specific patterns must precede more generic ones.
#  The analysis is performed until the first successful match.

package Polymake::Core::Shell::Completion;

my @line_completers = (

   new Completion(
      'qualified variable name',

      qr{ (?: $statement_start_re (?'cmd' local | (?:re)?set_custom) $args_start_re)?
          (?'sigil' $var_sigil_re)
          (?: (?'varname' $qual_id_re(?'qual'::)?)
              (?('scalar') (?('qual') | \s*\{\s* (?'keyquote' ['"])?(?'key' [\w-]*) )?) )? $}xo,

      sub {
         my ($shell) = @_;
         my ($cmd, $sigil, $varname, $keyquote, $key) = @+{qw(cmd sigil varname keyquote key)};

         if (defined($key)) {
            # completing a hash key
            if (my @custom_hash_topics = $User::application->help->find("custom", "%$varname")) {
               $shell->completion_proposals = [ sorted_uniq(sort( grep { index($_, $key)==0 } map { $_->name }
                                                                  map { @{$_->annex->{keys} // []} } @custom_hash_topics )) ];
            } elsif (defined(my $user_hash = find_user_variable('%', $varname))) {
               $shell->completion_proposals = [ sort( grep { index($_, $key)==0 } keys %$user_hash ) ];
            } else {
               $shell->completion_proposals = [ ];
            }
            $shell->completion_offset = length($key);
            if (@{$shell->completion_proposals} == 1) {
               $shell->completion_append_character = $keyquote || '}';
            }

         } else {
            # completing variable name
            my @custom_proposals = grep { $sigil eq '$' or $sigil eq '@' ? substr($_,0,1) ne '$' : substr($_,0,1) eq '%' }
                                   $User::application->help->list_completions("custom", ".$varname");
            my @proposals = map { s/^./$sigil/r } @custom_proposals;
            if ($cmd eq "" || $cmd eq "local"
                  and
                !@proposals || $varname !~ /::/) {
               push @proposals, complete_user_variable_name($sigil, $varname);
            }
            $shell->completion_proposals = [ sort(@proposals) ];
            $shell->completion_offset = length($sigil) + length($varname);
            if ($cmd && @proposals == 1 && substr($proposals[0], -1) ne ":") {
               if ($sigil eq '$' && @custom_proposals == 1 && substr($custom_proposals[0], 0, 1) ne '$') {
                  $shell->completion_append_character = substr($custom_proposals[0], 0, 1) eq '%' ? "{" : "[";
               } else {
                  $shell->completion_append_character = $cmd eq "reset_custom" ? ";" : "=";
               }
            }
         }
      },

      sub {
         my ($start_pos) = capturing_group_boundaries('args_start');
         $start_pos //= $-[0];
         my $varname = $+{varname};
         my $sigil = defined($+{key}) ? '%' : $+{sigil};
         my @proposals = grep { defined } $User::application->help->find("custom", qr{\Q$sigil$varname\E});
         ($start_pos, @proposals)
       }
   ),

   new Completion(
      'property name in a constructor call',

      qr{ $op_re new \s+ (?'type' $type_re) \s*\(\s* (?: $expression_re ,\s* )*+
                         (?: (?'text' $id_re) | $quote_re (?'text' $hier_id_re \.?)? )? $}xo,

      sub {
         my ($shell) = @_;
         my ($type, $quote, $text) = @+{qw(type quote text)};
         if (defined($type = $User::application->eval_type($type, 1))
               and
             instanceof BigObjectType($type)) {

            $shell->completion_proposals = [ try_property_completion($type, $text) ];
            $shell->completion_offset = length($text);
            if (defined($quote)) {
               $shell->completion_append_character = $quote;
            } else {
               $_ .= "=>" for @{$shell->completion_proposals};
            }
         }
      },

      sub {
         my ($type, $text) = @+{qw(type text)};
         # next time tell about the object type
         my $start_pos = (capturing_group_boundaries('type'))[0]+1;
         my @proposals;

         if (defined($type = $User::application->eval_type($type, 1))
               and
             instanceof BigObjectType($type)) {
            @proposals = fetch_property_help_or_warn($type, $text);
         }
         ($start_pos, @proposals);
      }
   ),

   new Completion(
      'type expression',

      qr{ $op_re (?'static' (?:new|typeof|instanceof) \s+)?+
                 (?'closed' $type_re (?<= >) \s*$)?+
                 (?('closed') | (?'open' (?: $qual_id_re \s*<\s* (?: $type_re \s*,\s*)*+ )+ )?
                                (?('static') | (?('open') | $ . ))   # if neither static nor open, provoke a mismatch
                                (?'expr' $type_re (?(?<=\w)::)? )? $)}xo,

      sub {
         my ($shell) = @_;
         my ($static, $open, $closed, $expr) = @+{qw(static open closed expr)};
         if (!$closed && $expr !~ /[<>]/) {
            # TODO: don't list the functions expecting BigObjectType literally (how to detect ???)
            my $only_objects = (!$static && $open =~ /^(?: cast )\b/) || undef;
            my ($app, $pkg);
            if (defined($app = try_type_completion($shell, $expr, $only_objects)) &&
                @{$shell->completion_proposals} == 1 &&
                defined($pkg = namespaces::lookup_class($app->pkg, $shell->completion_proposals->[0]))) {
               if (my $min_params = eval { $pkg->_min_params }) {
                  # the suggested type has itself mandatory type parameters
                  $shell->completion_append_character = "<";
               } elsif (!defined($min_params)) {
                  # the suggested type is simple, the outer type list may be closed or continued
                  if ($shell->partial_input !~ m{\G \s*[,>]}xgc &&
                      (my @delims = type_expr_delimiter($open, $closed, $static)) == 1) {
                     $shell->completion_append_character = $delims[0];
                  }
               }
            }
         } elsif ($shell->partial_input !~ m{\G \s*[,>]}xgc) {
            $shell->completion_proposals=[ type_expr_delimiter($open, $closed, $static) ];
         }
      },

      sub {
         my $start_pos = $-[0];
         my $open_start = (capturing_group_boundaries('open'))[0];
         my ($open, $expr) = @+{qw(open expr)};
         my ($innermost_open, $innermost_open_start, $innermost_open_end) =
            $open =~ /($qual_id_re) \s*[<,]\s* $/xo ? ($1, $-[1], $+[1]) : ();
         my @proposals;

         if (defined($expr)) {
            if ($expr =~ /^($qual_id_re) \s* (?: < | $ )/xo) {
               @proposals = fetch_type_help($1);
            }
            $start_pos = $innermost_open_end + $open_start if defined $innermost_open;
         } elsif (defined($innermost_open)) {
            @proposals = fetch_type_help($innermost_open);
            $start_pos = $innermost_open_start + $open_start;
         }
         ($start_pos, @proposals);
      }
   ),

   new Completion(
      'symbolic color name',

      qr{ \$Visual::Color::$id_re \s*=\s* (?: $quote_re (?!\d)(?'text' $non_quote_re*))? $}xo,

      sub {
         my ($shell) = @_;
         my ($quote, $text) = @+{qw(quote text)};

         if (defined($quote)) {
            $shell->completion_proposals = [ sort( Visual::list_color_completions($text) ) ];
            $shell->completion_offset = length($text);
            $shell->completion_append_character = $quote;
         } else {
            $shell->completion_proposals=[ '"' ];
         }
      }
      # no context help
   ),

   new Completion(
      'file status operator',

      qr{ $op_re (?<!-) -[a-zA-Z] \s+ $quote_re (?'text' $non_quote_re*) $}xo,

      sub {
         my ($shell) = @_;
         $shell->completion_append_character = $+{quote};
         $shell->completion_proposals = [ sort( try_filename_completion($shell, $+{text}) ) ];
      }
      # no context help
   ),

   new Completion(
      'external program execution',

      qr{ $op_re (?: qx ['"<(\[\{] (?'cmd' $balanced_re)) | \` (?'cmd' [^\`]* ) $}xo,

      sub {
         my ($shell) = @_;
         my $cmd = $+{cmd};
         $shell->completion_proposals = [ try_command_completion($shell, $cmd) ];
      }
      # no context help
   ),

   new Completion(
      'argument in a method call',

      qr{ $op_re $var_or_func_re $intermed_chain_re (?'last_method_name' $id_re) \s*\(\s* $arg_list_re $}xo,

      sub {
         my ($shell) = @_;
         my ($var, $app_name, $func, $intermed, $last_method_name, $preceding_args, $grouped_with, $keyword, $quote, $text)=@+{qw(
              var   app_name   func   intermed   last_method_name   preceding_args   grouped_with   keyword   quote   text)   };

         if (($var, my $type) = object_instance_and_type($var, $app_name, $func, $intermed)) {
            my $ctx = new Context($shell, $quote, $keyword, $preceding_args, $grouped_with, $var // $type);
            if (my @topics = uniq(retrieve_method_topics($type, $last_method_name, "find"))) {
               $shell->completion_offset = length($text) // 0;
               $shell->completion_append_character = $quote;
               $shell->completion_proposals = [ sorted_uniq(sort( try_param_completers($text, $ctx, "completion", @topics) )) ];
            }
         }
      },
      sub {
         my ($shell) = @_;
         my ($var, $app_name, $func, $intermed, $last_method_name, $preceding_args, $grouped_with, $keyword, $quote, $text)=@+{qw(
              var   app_name   func   intermed   last_method_name   preceding_args   grouped_with   keyword   quote   text)   };
         my $start_pos = $-[0];
         my $method_name_pos = (capturing_group_boundaries('last_method_name'))[0]+1;
         my @proposals;
         if (($var, my $type) = object_instance_and_type($var, $app_name, $func, $intermed)) {
            if (length($text)) {
               if (instanceof BigObjectType($type)
                     and
                   defined(my $prop = $type->lookup_property($last_method_name))) {
                  # property for selecting a subobject
                  if ($prop->flags & Property::Flags::is_multiple
                        and
                      @proposals = fetch_property_help_or_warn($prop->type, $text)) {
                     # tell about the subobject property, next time match the parent multiple property
                     return ($method_name_pos, @proposals);
                  }
               }
            }
            @proposals = uniq(retrieve_method_topics($type, $last_method_name, "find"));
            if (@proposals && length($text)) {
               my $ctx = new Context($shell, $quote, $keyword, $preceding_args, $grouped_with, $var // $type);
               if (my @arg_proposals = try_param_completers($text, $ctx, "help", @proposals)) {
                  # tell about the argument, next time match the method name
                  return ($method_name_pos, @arg_proposals);
               }
            }
         }
         # next time try matching enclosing function
         ($start_pos, @proposals)
      }
   ),

   new Completion(
      'argument in a free function call',

      qr{ $op_re $func_name_re (?: (?'tparams' (?=\s*<) $confined_re \s*\(\s* ) | $args_start_re) $arg_list_re $}xo,

      sub {
         my ($shell) = @_;
         my ($stmt, $app_name, $func, $tparams, $intermed, $preceding_args, $grouped_with, $keyword, $quote, $text)=@+{qw(
              stmt   app_name   func   tparams   intermed   preceding_args   grouped_with   keyword   quote   text)   };

         my ($app, @no_rel) = fetch_app($app_name) or return;
         my $ctx = new Context($shell, $quote, $keyword, $preceding_args, $grouped_with);
         $shell->completion_offset = length($text) // 0;
         $shell->completion_append_character = $quote;
         if (my @topics = $app->help->find(@no_rel, "functions", $func)) {
            $shell->completion_proposals = [ sorted_uniq(sort( try_param_completers($text, $ctx, "completion", @topics) )) ];
         } elsif ($quote && !$keyword) {
            my ($recognized, @proposals) = try_complete_standard_file_op($func, $shell, $preceding_args, $text);
            if ($recognized) {
               $shell->completion_proposals = [ sorted_uniq(sort @proposals) ];
            }
         }
      },
      sub {
         my ($shell) = @_;
         my ($stmt, $app_name, $func, $intermed, $preceding_args, $grouped_with, $keyword, $quote, $text)=@+{qw(
              stmt   app_name   func   intermed   preceding_args   grouped_with   keyword   quote   text)   };
         my @proposals;
         my $start_pos = $-[0];
         my $func_last_pos = (capturing_group_boundaries('func'))[1]-1;
         if (my ($app, @no_rel) = fetch_app($app_name)) {
            @proposals = uniq($app->help->find(@no_rel, "functions", $func));
            if (@proposals && length($text)) {
               my $ctx = new Context($shell, $quote, $keyword, $preceding_args, $grouped_with);
               if (my @arg_proposals = try_param_completers($text, $ctx, "help", @proposals)) {
                  # tell about the argument, next time match the function name
                  return ($func_last_pos, @arg_proposals);
               }
            }
         }
         # next time try matching enclosing function
         ($start_pos, @proposals)
      }
   ),

   new Completion(
      'method name',

      qr{ $op_re $var_or_func_re $intermed_chain_re (?'text' $id_re)? $}xo,

      sub {
         my ($shell) = @_;
         my ($var, $app_name, $func, $intermed, $text)=@+{qw(
              var   app_name   func   intermed   text)   };

         if (($var, my $type) = object_instance_and_type($var, $app_name, $func, $intermed)) {
            my @proposals = retrieve_method_topics($type, $text, "list_completions");
            if (ref($type) && $type->cppoptions && $type->cppoptions->fields) {
               push @proposals, grep { index($_, $text) == 0 } @{$type->cppoptions->fields};
            }
            $shell->completion_proposals = [ sorted_uniq(sort @proposals) ];
            $shell->completion_offset = length($text) // 0;
         }
      },
      sub {
         my ($shell) = @_;
         my ($var, $app_name, $func, $intermed, $text)=@+{qw(
              var   app_name   func   intermed   text)   };
         my $start_pos = $-[0];
         if (length($text) and defined(my $type = object_instance_and_type($var, $app_name, $func, $intermed))) {
            ($start_pos, uniq(retrieve_method_topics($type, $text, "find")))
         } else {
            $start_pos
         }
      }
   ),

   new Completion(
      'free function name',

      qr{ (?: (?'stmt' $statement_start_re) (?: (?'app_name' $id_re) ::)? (?'func' $id_re)?
            | $op_re (?: (?'app_name' $id_re) :: (?'func' $id_re)?
                       | (?'func' $id_re) )) $}xo,

      sub {
         my ($shell) = @_;
         my ($stmt, $app_name, $func) = @+{qw(stmt app_name func)};

         my ($type, @proposals,);
         my ($app, @no_rel) = fetch_app($app_name) or return;
         my @topics = $app->help->list_matching_topics(@no_rel, "functions", $func);
         unless (@no_rel || defined($stmt)) {
            # leave only non-void functions here;
            # but we can't take for granted that all user functions in applications are correctly annotated
            @topics = grep { defined($_->return_type) || $_->top->name ne "core" } @topics;
         }
         @proposals = map { $_->name } @topics;
         $shell->completion_offset = length($func) // 0;
         if (@topics == 1) {
            my $topic = $topics[0];
            my ($min_args, $max_args, $unlimited) = expects_params($topic);
            if ($max_args == 0 && !$unlimited) {
               $shell->completion_append_character = ";";
            } elsif ($min_args == 1 && $max_args == 1 && !$unlimited && !defined($topic->return_type) && $topic->top->name eq "core") {
               $shell->completion_append_character = " ";
            } elsif (($min_args, $max_args) = expects_template_params($topic)) {
               if ($min_args > 0) {
                  $shell->completion_append_character = "<";
               }
            } else {
               $shell->completion_append_character = "(";
            }
         } elsif (!defined($app_name)) {
            push @proposals, matching_app_prefixes($shell, $app, $func);
         }
         if (@proposals) {
            $shell->completion_proposals = [ sorted_uniq(sort @proposals) ];
         }
      },

      sub {
         my ($shell) = @_;
         my ($app_name, $func)=@+{qw(app_name func)};
         my $start_pos = $-[0];
         if (length($func) and my ($app, @no_rel) = fetch_app($app_name)) {
            ($start_pos, uniq($app->help->find(@no_rel, "functions", $func)))
         } else {
            $start_pos
         }
      }
   ),
);

################################################################################################
#
#  Top level routines for the Shell
#

sub get_completion {
   my ($shell, $pos) = @_;
   pos($shell->partial_input) = $pos;
   foreach my $completer (@line_completers) {
      if (substr($shell->partial_input, 0, $pos) =~ /${$completer->pattern}/) {
         $completer->completion->($shell);
         return if defined($shell->completion_proposals);
      }
   }
   # enforce filename completion on the next try?
   if ($shell->partial_input =~ s{^ $quote_balanced_re $quote_re (?'text' $non_quote_space_re*) $}{$+{text}}xo) {
      $shell->completion_append_character = $+{quote};
   } else {
      $shell->completion_proposals = [ ];
   }
}

sub get_help_topics {
   my ($shell, $pos) = @_;
   $#{$shell->help_topics} = -1;
   pos($shell->partial_input) = $pos;
   $pos = 0;

   do {
      # move before the end of the statement if nothing follows
      $shell->partial_input =~ m{(?: \)\s* (?: ;\s*)? | \s*;\s* ) \G \s*$ }xgc
        and
      pos($shell->partial_input) = $-[0];

      # move before the trailing spaces and operator signs unless at the beginning of a name
      $shell->partial_input =~ m{[-+*/=&|^\s]+ \G (?![a-zA-Z])}xgc
        and
      pos($shell->partial_input) = $-[0];

      # move before the closing quote of a string if standing immediately behind it
      $shell->partial_input =~ m{^ $quote_balanced_re (?<= $anon_quote_re) \G}xogc
        and
      pos($shell->partial_input) = pos($shell->partial_input)-1;

      # move before the keyword arrow if standing immediately behind it
      $shell->partial_input =~ m{ ($id_re) \s* => \G }xogc
        and
      pos($shell->partial_input) = $+[1];

      # move past the name if currently pointing in its middle
      $shell->partial_input =~ m{($qual_id_re)? \G (?(1) [\w:]* | $qual_id_re)}xogc;

      foreach my $completer (@line_completers) {
         if (defined($completer->help)
               and
             substr($shell->partial_input, 0, pos($shell->partial_input)) =~ /${$completer->pattern}/
               and
             ($pos, @{$shell->help_topics}) = $completer->help->($shell)) {
            if ($pos >= pos($shell->partial_input)) {
               croak( "internal error:\ncompleter '", $completer->name, "' requested to move position from\n",
                      substr($shell->partial_input, 0, pos($shell->partial_input)), "<<*>>", substr($shell->partial_input, pos($shell->partial_input)), "\nto\n",
                      substr($shell->partial_input, 0, $pos), "<<*>>", substr($shell->partial_input, $pos), "\n");
            }
            pos($shell->partial_input) = $pos;
            return 1 if @{$shell->help_topics};
            last;
         }
      }
   } while ($pos > 0);
   0
}

################################################################################################
#
#  Helper routines for line completers
#

sub matching_app_prefixes {
   my ($shell, $app, $text) = @_;
   if (my @app_names = grep { /^\Q$text\E/ && !$app->imported->{$_} } keys %{$app->used}) {
      # complete application name as qualification prefix
      undef $shell->completion_append_character;
      map { $_."::" } @app_names;
   } else {
      ()
   }
}

sub fetch_app {
   my ($name) = @_;
   if (length($name) == 0) {
      $User::application
   } elsif (defined(my $app = eval { add Application($name) })) {
      ($app, "!rel")
   } else {
      ()
   }
}

sub prepare_property_lookup {
   my ($type, $text) = @_;
   my @path = split /\./, $text;
   $text = substr($text, -1, 1) eq "." ? "" : pop @path;
   foreach (@path) {
      my $prop = $type->lookup_property($_) or return;
      $prop->flags & Property::Flags::is_subobject or return;
      $type = $prop->type;
   }
   $_[0] = $type;
   $_[1] = $text;
   1
}

sub try_property_completion {
   my ($type, $text, $multi) = @_;
   if (prepare_property_lookup($type, $text)) {
      $_[1] = $text;
      sorted_uniq(sort( grep { !$type->is_overridden($_) }
                        map { $_->name }
                        grep { $_->name =~ /^\Q$text\E(?!.*\.pure$)/ &&
                               not($_->flags & Property::Flags::is_permutation) &&
                               (!$multi || $_->flags & Property::Flags::is_multiple) }
                        map { values %{$_->properties} } $type, @{$type->linear_isa} ))
   } else {
      ()
   }
}

sub fetch_property_help_or_warn {
   if (my @proposals = &fetch_property_help) {
      @proposals
   } else {
      my ($type, $text) = @_;
      sub { print "no such property '$text' in object type ", $type->full_name, "\n" }
   }
}

sub fetch_property_help {
   my ($type, $text) = @_;
   $text =~ s/\.$//;
   if (prepare_property_lookup($type, $text)) {
      my $topic;
      uniq( grep { !$type->is_overridden($_->name) }
            map { defined($topic = $_->help_topic) ? $topic->find("!rel", "properties", $text) : () }
                $type, @{$type->linear_isa} )
   } else {
      ()
   }
}

sub prepare_type_matching {
   my ($text, $is_object_type) = @_;
   my $maybe_object_type = !( $text =~ s/\bprops::// ) && (!defined($is_object_type) || $is_object_type);
   my $maybe_prop_type = !( $text =~ s/\bobjects::// ) && !$is_object_type;
   my $app_name = $text =~ s/^($id_re)::// && $1;
   ($text, $app_name, $maybe_object_type, $maybe_prop_type)
}

sub try_type_completion {
   my ($shell, $text, $is_object_type) = @_;
   my ($type_prefix, $app_name, @maybe) = prepare_type_matching($text, $is_object_type);
   my ($app, @no_rel) = fetch_app($app_name) or return;
   my @proposals;
   foreach my $whence (qw(objects property_types)) {
      if (shift @maybe) {
         push @proposals, $app->help->list_completions(@no_rel, $whence, $type_prefix);
      }
   }
   $shell->completion_offset = length($type_prefix);
   if (@proposals) {
      $shell->completion_proposals = [ sorted_uniq(sort(@proposals)) ];
      $app
   } else {
      if ($text !~ /::/) {
         $shell->completion_proposals = [ map { "$_\::" } sort( grep { /^\Q$text\E/ } keys %{$app->used}) ];
      }
      undef
   }
}

sub type_expr_delimiter {
   my ($open, $closed, $static) = @_;
   my $type_expr = $open // $closed =~ s/>$/,/r;   # for the ease of type param list parsing
   my @proposals;
   if (length($type_expr)) {
      my $cnt = 1;
      ++$cnt while $type_expr =~ s/$type_re \s*,\s* $//xo;
      $type_expr =~ s/($qual_id_re) \s*<\s* $//xo;
      my $outer = $1;
      my ($min_params, $max_params);
      if (!$static && length($type_expr)==0) {
         # a parametrized function
         my ($app, @no_rel) = ($outer =~ s/^($id_re)::// ? fetch_app($1) : $User::application) or return;
         if (defined(my $h = $app->help->find(@no_rel, "functions", $outer))) {
            ($min_params, $max_params) = expects_template_params($h);
         }
      } else {
         # a parametrized type
         if (defined(my $pkg = namespaces::lookup_class($User::application->pkg, $outer))) {
            ($min_params, $max_params) = eval { ($pkg->_min_params, scalar(@{$pkg->typeof_gen->params})) };
         }
      }
      if (length($closed))  {
         if (defined($min_params) && $cnt >= $min_params && $static ne "typeof") {
            # start an argument list
            push @proposals, "(";
         }
      } else {
         if (defined($max_params) && $cnt < $max_params) {
            push @proposals, ",";
         }
         if (defined($min_params) && $cnt >= $min_params) {
            push @proposals, ">";
         }
      }
   }
   @proposals
}

sub fetch_type_help {
   my ($text, $is_object_type)=@_;
   my ($type_name, $app_name, @maybe) = prepare_type_matching($text, $is_object_type);
   my @proposals;
   if (my ($app, @no_rel) = fetch_app($app_name)) {
      foreach my $whence (qw(objects property_types)) {
         if (shift @maybe) {
            push @proposals, $app->help->find(@no_rel, $whence, $type_name);
         }
      }
   }
   uniq(@proposals);
}

# -> min, max, unlimited
sub expects_params {
   my ($topic) = @_;
   $topic->map_reduce_for_function(
      sub {
         if (defined(my $params = $_->{param})) {
            my $mandatory = $_->{mandatory};
            (defined($mandatory) ? $mandatory+1 : scalar @$params, scalar @$params, $_->{ellipsis} || defined($_->{options}))
         } else {
            ()
         }
      },
      sub {
         my ($result, $min, $max, $unlimited) = @_;
         assign_min($result->[0], $min);
         assign_max($result->[1], $max);
         $result->[2] ||= $unlimited;
      })
}

# => min, max
sub expects_template_params {
   my ($topic) = @_;
   $topic->map_reduce_for_function(
      sub {
         if (defined(my $tparams = $_->{tparam})) {
            ($_->{mandatory_tparams}, scalar @$tparams)
         } else {
            ()
         }
      },
      sub {
         my ($result, $min, $max) = @_;
         assign_min($result->[0], $min);
         assign_max($result->[1], $max);
      })
}

sub retrieve_return_type {
   my ($topic) = @_;
   my $type = $topic->return_type;
   if (defined($type) and $type !~ /::/) {
      $topic = $topic->top;
      if ($topic->name ne "core") {
         my $app = User::application($topic->name);
         $type = $app->eval_type($type, 1) // ($type =~ s/^($id_re)<.*>$/$1/ and $app->eval_type($type, 1));
      }
   }
   $type
}

sub object_instance_and_type {
   my ($var, $app_name, $func, $intermed) = @_;
   my ($app, $type, $topic, @norel);

   if (defined($var)) {
      if ($intermed =~ s/^(?:\s*->)?\s*\[\s*-?\d+\s*\]//) {
         # array element with a fixed index
         $var .= $&;
      }
      package Polymake::User;
      $type = eval "$var->type";
      $type //= eval "ref($var)";
      if (defined($type)) {
         $var = eval $var;
      } else {
         undef $var;
      }
   } else {
      if (defined($app_name)) {
         $app = eval { User::application($app_name) } or return;
         @norel = qw(!rel);
      } else {
         $app = $User::application;
      }
      if (defined($func = $app->help->find(@norel, "functions", $func))) {
         $type = retrieve_return_type($func);
      }
   }

   if (defined($type)) {
      my $prop;
      while ($intermed =~ m{\G $intermed_re }gcxo) {
         if (defined(my $method_name = $+{method_name})) {
            undef $prop;
            if (instanceof BigObjectType($type) and defined($prop = $type->lookup_property($method_name))) {
               $type = $prop->type;
               if (defined($var)) {
                  if ($prop->flags & Property::Flags::is_multiple) {
                     if (defined(my $call_args = $+{call_args})) {
                        if ($call_args =~ /^\(\s* $quoted_re \s*\)$/xo) {
                           $var = $var->lookup($prop->name, $+{quoted});
                        } else {
                           # too complicated to evaluate during tab completion - might have side effects
                           undef $var;
                        }
                     } elsif ($intermed =~ m{\G \s*->\s*\[(\d+)\] }gcxo) {
                        if (defined(my $values = $var->lookup($prop->name, '*'))) {
                           $var = $values->[$1];
                        } else {
                           undef $var;
                        }
                     } else {
                        $var = $var->lookup($prop->name);
                     }
                  } else {
                     $var = $var->lookup($prop->name);
                  }
               }
            } elsif (defined($func = ref($type) ? ($topic = $type->help_topic  and  $topic->find("methods", $method_name))
                                                : ($topic = $User::application->help->find("objects", $type)  and  $topic->find("methods", $method_name)))) {
               if (defined(my $ret_type = retrieve_return_type($func))) {
                  $type = $ret_type;
               }
               undef $var;
               # otherwise let's just suppose the method returns the same object
            } else {
               ref($type) or defined($type = $User::application->eval_type($type, 1)) or return;
               (instanceof PropertyType($type) and $type = $type->get_field_type($method_name)) or return;
               undef $var;
            }
         } else {
            $type = $type->get_array_element_type or return;
            undef $var;
         }
      }
      $intermed =~ m{\G \s*$}gx or return;
   }

   ($var, $type)
}

sub core_object_topic {
   state $core_object_topic = $Help::core->find(qw(objects Core::BigObject));
}

sub retrieve_method_topics {
   my ($type, $name, $help_method) = @_;
   if (ref($type)) {
      if (instanceof BigObjectType($type)) {
         my @object_topics = grep { defined } map { $_->help_topic } $type, @{$type->linear_isa};
         ( grep { !$type->is_overridden($help_method eq "find" ? $_->name : $_) }
           map { $_->$help_method("!rel", "properties", $name) } @object_topics ),
         ( map { $_->$help_method("!rel", "methods", $name) } @object_topics, core_object_topic() )
      } else {
         my @proposals;
         do {
            if (defined(my $topic = $type->help_topic)) {
               push @proposals, $topic->$help_method("!rel", "methods", $name);
            }
            $type = $type->super;
         } while (defined($type));
         @proposals
      }
   } else {
      no strict 'refs';
      my @proposals = map {
            map { $_->$help_method("!rel", "methods", $name) } $User::application->help->find("objects", $_)
         } $type, @{"$type\::ISA"};
      if (!@proposals && $type =~ s/^Polymake::((?:Core::)?$id_re)$/$1/) {
         @proposals = map { $_->$help_method("methods", $name) } $Help::core->find("objects", $type);
      }
      @proposals
   }
}

sub try_param_completers {
   my ($text, $ctx, $method, @topics) = @_;
   my $preceding_args = @{$ctx->preceding_args};
   my @proposals;
   foreach my $topic (@topics) {
      my $arg_num = $preceding_args;
      if (defined($ctx->obj) && defined(my $prop = $topic->annex->{property})) {
         state $core_object_give_topic = core_object_topic()->find("methods", "give");
         $topic = $core_object_give_topic or next;
         $ctx->prop_name = $prop->name;
         ++$arg_num;
      }
      $topic->map_reduce_for_function(
         sub {
            my $keyword = $ctx->group_keyword // $ctx->keyword;
            if (!$keyword && defined(my $params = $_->{param})) {
               if (my $param = $arg_num < @$params ? $params->[$arg_num] : $_->{ellipsis} && @$params && $params->[-1]) {
                  # completion of a positional parameter
                  push @proposals, try_param_completer($text, $ctx, $method, $param);
               }
            }
            if ($method eq "completion" && defined(my $options = $_->{options}) && $arg_num > ($_->{mandatory} // $arg_num)) {
               if ($keyword) {
                  # completion of a value for a known keyword
                  foreach my $param (grep { $_->name eq $keyword } map { @{$_->annex->{keys}} } map { $_, @{$_->related} } @$options) {
                     push @proposals, try_param_completer($text, $ctx, $method, $param);
                  }
               } else {
                  # completion of a keyword
                  push @proposals, map { $ctx->quote ? $_ : "$_=>" }
                       grep { /^\Q$text\E/ } map { $_->name } map { @{$_->annex->{keys}} } map { $_, @{$_->related} } @$options;
               }
            }
            ()
         });
   }
   @proposals
}

sub try_param_completer {
   my ($text, $ctx, $method, $param) = @_;
   my $completer = $param->completer
     or return;
   $completer->$method
     or return;
   if ($completer->flags & Param::Flags::object && !$ctx->is_real_object) {
      return;
   }
   if ($param->completer_tag eq "grouped"
       ? $param->type ne ref($ctx->grouped_with)
       : defined($ctx->grouped_with)) {
      if (!$ctx->quote && $param->completer_tag eq "grouped" && length($text) == 0 && $method eq "completion") {
         if ($param->type eq "HASH") {
            return "{";
         }
         if ($param->type eq "ARRAY") {
            return "[";
         }
      }
      return;
   }
   if (!$ctx->quote and !defined($ctx->grouped_with) && $completer->flags & Param::Flags::quoted) {
      return length($text) == 0 && $method eq "completion" ? '"' : ();
   }
   if (!defined($completer->pattern) || $text =~ /${$completer->pattern}/) {
      $ctx->values = $param->values;
      $ctx->tag = $param->completer_tag;
      defined($ctx->grouped_with) && !$ctx->quote && !$ctx->keyword
      ? (map { "$_=>" } $completer->$method->($text, $ctx))
      : $completer->$method->($text, $ctx)
   } else {
      ()
   }
}

################################################################################################
#
#  variable name completion
#

sub complete_variable_name_in_pkg {
   my ($pkg, $sigil, $prefix) = @_;
   my @proposals;
   while (my ($name, $glob) = each %$pkg) {
      if (length($prefix) ? index($name, $prefix) == 0 : $name !~ /^\./) {
         if (defined_scalar($glob) && $sigil eq '$' or
             defined(*{$glob}{ARRAY}) && ($sigil eq '@' || $sigil eq '$') or
             defined(*{$glob}{HASH}) && ($sigil eq '%' || $sigil eq '$' || $name =~ /::$/)) {
            push @proposals, $sigil.$name;
         }
      }
   }
   @proposals
}

sub complete_user_variable_name {
   my ($sigil, $prefix) = @_;
   if ((my $pkg_end = rindex($prefix,"::")) > 0) {
      my $pkg = substr($prefix, 0, $pkg_end);
      my $symtab = eval { get_symtab("Polymake::User::$pkg") } or return;
      map { "$pkg\::$_" } complete_variable_name_in_pkg($symtab, $sigil, substr($prefix, $pkg_end+2));
   } else {
      complete_variable_name_in_pkg(\%Polymake::User::, @_);
   }
}

sub find_user_variable {
   my ($type, $name)=@_;
   my $glob=do {
      if ((my $pkg_end=rindex($name, "::"))>0) {
         my $symtab=eval { get_symtab("Polymake::User::" . substr($name,0,$pkg_end)) } or return;
         $symtab->{substr($name,$pkg_end+2)};
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

################################################################################################
#
#  filename completion
#

my %standard_file_ops = (
   chdir => [ 0, 0, "dir" ],
   chmod => [ 1 ],
   chown => [ 2 ],
   glob => [ 0, 0 ],
   link => [ 0, 1 ],
   lstat => [ 0, 0 ],
   mkdir => [ 0, 0, "dir" ],
   open => [ 1, 2 ],
   opendir => [ 1, 1, "dir" ],
   readlink => [ 0, 0 ],
   readpipe => [ 0, 0, "cmd" ],
   rename => [ 0, 1 ],
   rmdir => [ 0, 0, "dir" ],
   stat => [ 0, 0 ],
   symlink => [ 0, 1 ],
   system => [ 0, 0, "cmd" ],
   unlink => [ 0 ],
   utime => [ 2 ]
);

sub try_complete_standard_file_op {
   if (defined(my $descr = $standard_file_ops{$_[0]})) {
      (true, check_standard_file_op($descr, @_))
   } else {
      (false)
   }
}

sub check_standard_file_op {
   my ($descr, $func, $shell, $preceding_args, $text) = @_;
   my $arg_num = 0;
   while ($preceding_args =~ /\G ($expression_re) ,\s* /xog) {
      ++$arg_num;
   }
   return if $arg_num < $descr->[0] || $arg_num > ($descr->[1] // $arg_num);
   if ($descr->[2] eq "cmd") {
      return try_command_completion($shell, $text);
   }
   if ($func eq "open") {
      if ($text =~ m{^\s*\|}) {
         return try_command_completion($shell, $text);
      }
      if ($arg_num == 1) {
         # strip off open mode if concatenated with filename
         $text =~ s{^(?:>>?|\+?<)}{};
      }
   }
   try_filename_completion($shell, $text, $descr->[2])
}

sub try_filename_completion {
   my ($shell, $text, $mode, @where) = @_;
   $shell->completion_offset = length($text);
   map {
      my $location = $_;
      if (my @list = $shell->term->completion_matches(defined($location) ? "$location/$text" : $text,
                                                      $shell->term->Attribs->{filename_completion_function})) {
         shift @list if $#list;
         if (defined($location)) {
            my $tail = length($location)+1;
            map { substr($_, $tail) } ($mode eq "exec" ? (grep { -x -f } @list) : @list)
         } elsif ($mode eq "dir") {
            unshift @list, ".." if length($text) == 0 || $text =~ /^\.\.?$/;
            $shell->completion_append_character = "/";
            grep { m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ } @list
         } else {
            map {
               my $file = m|^~/| ? "$ENV{HOME}/$'" : $_;
               if (-d $file) {
                  undef $shell->completion_append_character;
                  "$_/"
               } elsif ($mode eq "exec" && !(-x -f $file)) {
                  ()
               } else {
                  $_
               }
            } @list
         }
      } else {
         ()
      }
   } (undef, @where)
}

my $shell_cmd_start_re = qr{^ | (?<!\\\\) (?: [|&(\{] | ; (?: \s* (?: then | else | do ))?) }x;
my $shell_special_chars = q% ;|&(){}\[\]$'"\\?*!<>%;
my $shell_non_special_char_re = qr{[^$shell_special_chars]};
my $shell_escaped_char_re = qr{(?<!\\\\) \\\\ [$shell_special_chars] | $shell_non_special_char_re}xo;

sub try_command_completion {
   my ($shell, $cmd) = @_;
   if ($cmd =~ /$shell_cmd_start_re \s* ($shell_non_special_char_re+)$/xo) {
      # program name
      $cmd = $1;
      $shell->completion_append_character = " ";
      if ($cmd =~ m{^[/~]}) {
         sort( try_filename_completion($shell, $cmd, "exec") )
      } else {
         sorted_uniq(sort( try_filename_completion($shell, $cmd, "exec", split /:/, $ENV{PATH}) ))
      }
   } elsif ($cmd =~ /^(?= \s*\S) $quote_balanced_re (?<!\\\\)\s (?: $quote_re (?'text' $non_quote_re*) | (?'text' (?:$shell_escaped_char_re)*)) $/xo) {
      # filename argument
      my $text = $+{text};
      if ($+{quote}) {
         $shell->completion_append_character = $+{quote};
         sort( try_filename_completion($shell, $text) )
      } else {
         my @proposals = try_filename_completion($shell, $text =~ s/(?<!\\)\\\\(?!\\)//gr);
         $shell->completion_offset = length($text);
         sort( map { s/(?<!\\\\)[$shell_special_chars]/\\\\$&/gr } @proposals )
      }
   } else {
      ()
   }
}

################################################################################################
#
#  Extensions can add own completers using these methods
#

# Append one or more line completers at the end of the list
sub append {
   shift;   # package name
   push @line_completers, @_;
}

# Insert one or more line completers before the one with the given name
sub insert_before {
   shift;   # package name
   my $before_name = shift;
   for (my $i = 0; $i <= $#line_completers; ++$i) {
      if ($line_completers[$i]->name eq $before_name) {
         splice @line_completers, $i, 0, @_;
         return;
      }
   }
   croak( "No shell completer found with name matching $before_name" );
}

package Polymake::Core::Shell::Completion::Param;

sub add {
   my ($pkg, $name, @args) = @_;
   ($completers{$name} &&= croak("parameter completer $name already exists")) = new(__PACKAGE__, @args);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
