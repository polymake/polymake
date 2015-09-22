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
use feature 'state';

#################################################################################
package Polymake::Core;

sub multiple_func_definition {
   croak( "Multiple definition of a non-overloaded function" );
}

sub check_type_definition {
   my $expected=shift;
   my $pkg_entry=shift;
   foreach (@_) {
      unless (defined ($pkg_entry=$pkg_entry->{$_."::"})) {
         if ($expected) {
            croak( "Unknown ", $_[0] eq "props" ? "property" : "`big' object", " type" );
         }
         return;
      }
   }
   if (defined (my $type_stash=*{$pkg_entry}{HASH})) {
      if (exists $type_stash->{typeof}) {
         return if $expected;
         croak( "Multiple definition of ", $_[0] eq "props" ? "a property type" : @_==2 ? "an object type" : "a named type specialization" );
      }
   }
   if ($expected) {
      croak( "Unknown ", $_[0] eq "props" ? "property type" : @_==2 ? "`big' object type" : "type specialization");
   }
}

sub multiple_prop_definition { check_type_definition(0, \%application::, "props", @_) }
sub multiple_object_definition { check_type_definition(0, \%application::, "objects", @_) }

sub check_proper_app_use {
   my ($app, $other_app_name)=@_;
   $app->used->{$other_app_name}
     or lookup Application($other_app_name)
        ? croak( "Unknown application $other_app_name" )
        : croak( "An application can only enhance own object/property types or those defined in an IMPORT'ed or USE'd application" );
}

sub check_application_pkg {
   compiling_in(\%application::) == $_[0]
     or croak( "This declaration ",
               $_[0] ? "is only allowed" : "is not allowed",
               " in the top-level (application) scope" );
}

sub check_object_pkg {
   $_[0] or croak( "This declaration is only allowed in the top-level (application) scope" );
}

declare $warn_options="use strict 'refs'; use warnings 'void', 'syntax'; use feature 'state'";

#################################################################################
package Polymake::Core::ScriptLoader;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$handle' => '#1' ],
   [ '$prologue' => '#2' ],
);

sub new {
   my $self=&_new;
   (\&get, $self);
}

sub get {
   my ($maxlen, $self)=@_;
   if (@{$self->prologue}) {
      $_ .= shift @{$self->prologue};
   } else {
      $_ .= readline $self->handle;
   }
   return length;
}
#################################################################################
package Polymake::Core::ScriptFilter;

use Polymake::Struct (
   [ '@ISA' => 'ScriptLoader' ],
   [ new => '$$@' ],
   [ '@conv' => '@' ],
);

sub new {
   my $self=&_new;
   (\&get, $self);
}

sub get {
   my ($maxlen, $self)=@_;
   if (@{$self->prologue}) {
      $_ .= shift @{$self->prologue};
   } else {
      namespaces::temp_disable();
      my $line=readline $self->handle;
      foreach my $conv (@{$self->conv}) {
         last if $conv->($line);
      }
      $_ .= $line;
   }
   return length;
}
#################################################################################
#
#   Rule parser

my $funcnt="aaa000";
my $accurate_linenumbers= exists &DB::DB || namespaces::collecting_coverage();

package Polymake::Core::Application::RuleFilter;

my (%main_init_rule_headers, %rule_headers, %main_init_decl_headers, %decl_headers, %cross_app_rule_headers, %cross_app_decl_headers);

use Polymake::Struct (
   [ new => '$$$$$$$@' ],
   [ '$handle' => '#1' ],
   [ '$application' => '#2' ],
   [ '$path' => '#3' ],
   [ '$rule_key' => '#4' ],
   [ '$from_embedded_rules' => '#5' ],
   '@buffer',
   [ '$buffer_phase1' => 'undef' ],
   [ '$buffer_suspended' => 'undef' ],
   [ '$gap' => '1' ],           # 1 - empty line, 2 - comment block
   '$start_comments',
   '$len_comments',
   [ '$credit_seen' => '#6' ],
   '$after_rule',
   '@trailer',
   '&filter',
   '$header_line',
   [ '$preamble_end' => '0' ],
   [ '$prolonged' => '0' ],
   [ '$injected_lines' => 'undef' ],
   [ '$injected_source' => 'undef' ],
   [ '$rule_header_table' => 'undef' ],
   [ '$decl_rule_header_table' => 'undef' ],
);

sub new {
   my $self=&_new;
   splice @_, 0, 6;
   if (shift) {
      $self->rule_header_table=\%main_init_rule_headers;
      $self->decl_rule_header_table=\%main_init_decl_headers;
   } else {
      $self->rule_header_table=\%rule_headers;
      $self->decl_rule_header_table=\%decl_headers;
   }
   @{$self->buffer}= @_>1 ? @_ : split /(?<=\n)/, shift;

   if ($self->from_embedded_rules) {
      my $app_pkg=$self->application->pkg;
      push @{$self->buffer_phase1=[ ]},
           "use namespaces '$app_pkg';\n",
           "package $app_pkg;\n";
      $self->application->cpp->embedded_rules=$self->buffer;
      (\&get_embedded_rules, $self);

   } else {
      if ($accurate_linenumbers) {
         $self->injected_lines=@{$self->buffer}+1;
         $self->application =~ /0x[0-9a-f]+/;
         $self->injected_source="/loader/$&/rules:".$self->path;
      }
      (\&get, $self);
   }
}

sub get {
   my ($maxlen, $self)=@_;
   unless (@{$self->buffer}) {
      namespaces::temp_disable();
      fill($self);
   }
   print STDERR ">>> ", $self->buffer->[0] if $DebugLevel>3;
   $_ .= shift @{$self->buffer};
   return length;
}

sub get_embedded_rules {
   my ($maxlen, $self)=@_;
   unless (@{$self->buffer_phase1}) {
      namespaces::temp_disable();
      do {
         fill($self);
         if (defined($self->buffer_suspended) && $self->buffer->[-1] =~ /^\}(?: \{)?$/) {
            # end of a block suspended because of unsatisfied REQUIRE_APPLICATION
            push @{$self->buffer_suspended},
                 "  undef;\n",
                 splice @{$self->buffer}, -1, 1, "}\n";
            $self->buffer=$self->buffer_suspended;
            undef $self->buffer_suspended;

            # reset the (decl) rule header table
            # it was set to cross_app_* at the beginning of the REQUIRE_APPLICATION block
            $self->rule_header_table=\%rule_headers;
            $self->decl_rule_header_table=\%decl_headers;
         }
      } until (@{$self->buffer_phase1});
   }

   print STDERR "++> ", $self->buffer_phase1->[0] if $DebugLevel>3;
   $_ .= shift @{$self->buffer_phase1};
   return length;
}
#################################################################################
# Insert "typeof" method calls in front of every type expression
sub translate_type_expr {
   $_[0] =~ s{ (?: ^ | [\(\{,?:] )\s* \K (?! typeof | instanceof | undef) ($type_re) (?! \s*(?: [\(\[\{\$\@%<>] | -> | => )) }{(typeof $1)}xog;
   $_[0] =~ s/^\s*(?=\{)/do /;
}
#################################################################################
# these clauses are recognized at the beginning of main.rules and elsewhere

%main_init_rule_headers=(
   IMPORT => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      push @{$self->buffer}, "BEGIN { application::self()->use_apps(1, qw($header\n";
      push @{$self->trailer}, ")) }\n";
   },

   USE => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      push @{$self->buffer}, "BEGIN { application::self()->use_apps(0, qw($header\n";
      push @{$self->trailer}, ")) }\n";
   },

   HELP => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      push @{$self->buffer}, ($Help::gather ? "application::self()->include_rule_block(0, " : "0 and (")."q($header\n";
      push @{$self->trailer}, "));\n";
   },

   CREDIT => sub {
      my ($self, $header)=@_;
      if ($header =~ /^\s*(?:-|none|off|(default))\s*$/) {
         my $credit_val= $1 && $extension && '=$Polymake::Core::Application::extension->credit';
         $self->credit_seen= $credit_val && defined($extension->credit);
         push @{$self->buffer},
              "my \$__cur_credit$credit_val;\n";
      } elsif (my ($product, $ext_URI)= $header =~ /^\s* (\w+) (?: \s*=\s* (\S+))? \s*$/ox) {
         $self->credit_seen=1;
         push @{$self->buffer},
              "my \$__cur_credit=application::self()->add_credit('$product', '$ext_URI', <<'_#_#_#_', '" . $self->rule_key . "');\n";
         push @{$self->trailer}, "_#_#_#_\n";
         $self->application->credits_by_rulefile->{$self->rule_key}=$product;
      } else {
         push @{$self->buffer},
              "BEGIN { die 'invalid product name in credit definition' }\n";
      }
   },

   file_suffix => sub {
      my ($self, $header)=@_;
      if ($header =~ /(\w+)\s*$/) {
         push @{$self->buffer},
              "self(-2)->set_file_suffix('$1');\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid file suffix' }\n";
      }
   },
);

# these clauses automatically close the initial preamble

%rule_headers=(
   IMPORT => sub {
      my ($self, $header)=@_;
      push @{$self->buffer}, "BEGIN { die 'IMPORT clause may only appear at the beginning of main.rules where the application is introduced' }\n";
   },

   INCLUDE => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      push @{$self->buffer}, "application::self()->include_rule_block(0, q($header\n";
      push @{$self->trailer}, "));\n";
   },

   REQUIRE => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      my $escape= $self->from_embedded_rules ? "last" : "return 1";
      push @{$self->buffer},
           &start_preamble . "->include_required(q($header\n";
      push @{$self->trailer},
           "), '" . $self->rule_key . "', $.) || $escape;\n";
   },

   REQUIRE_EXTENSION => sub {
      my ($self, $header)=@_;
      $self->filter=\&erase_comments_filter;
      $header =~ s/\#.*$//;
      my $escape= $self->from_embedded_rules ? "last" : "return 1";
      push @{$self->buffer},
           &start_preamble . "->require_ext(q($header\n";

      push @{$self->trailer},
           "), '" . $self->rule_key . "', $.) || $escape;\n";
   },

   REQUIRE_APPLICATION => sub {
      my ($self, $header)=@_;
      $header =~ s/\#.*$//;
      if (my @app_names= $header =~ /\G\s* ($id_re) \s*/gxoc
            and
          pos($header)==length($header)) {
         my @missing_apps;
         foreach my $app_name (@app_names) {
            if (lookup Application($app_name)) {
               if ($plausibility_checks && exists $self->application->used->{$app_name}) {
                  push @{$self->buffer},
                       "BEGIN { die 'superfluous REQUIRE_APPLICATION $app_name: already USE\\'d or IMPORT\\'ed' }\n";
                  return;
               }
            } else {
               push @missing_apps, $app_name;
            }
         }
         if (@missing_apps) {
            my $suspended=Application::SuspendedItems::add($self->application, $extension, @missing_apps);
            if ($self->from_embedded_rules) {
               # switch temporarily to a separate line buffer; get_embedded_rules will switch back later
               $self->buffer_suspended //= $self->buffer;
               $self->buffer=$suspended->embedded_rules;
               push @{$self->buffer},
                    "{\n",
                    "#line $. \"" . tied(*{$self->handle})->lastfile . "\"\n";
            } else {
               # stop loading now
               push @{$suspended->rulefiles}, $self->path;
               push @{$suspended->rule_keys}, $self->rule_key;
               push @{$self->buffer},
                    "1; __END__\n";
               return;
            }
         }

         # C++ functions defined in the sequel must memorize the application list
         push @{$self->buffer},
              "local \$Polymake::Core::Application::cross_apps_list=[ map { lookup Polymake::Core::Application(\$_) } qw(@app_names) ];\n";

         $self->rule_header_table=\%cross_app_rule_headers;
         $self->decl_rule_header_table=\%cross_app_decl_headers;

      } else {
         push @{$self->buffer},
              "BEGIN { die 'invalid application list' }\n";
      }
   },

   CONFIGURE => sub { process_configure(@_[0,1], 0) },

   CONFIGURE_OPT => sub { process_configure(@_[0,1], 1) },

   CONFIGURE_BUILD => sub {
      my ($self, $header)=@_;
      if ($extension) {
         ++$funcnt;
         $header =~ s/^\s*\{/sub __unused_conf_$funcnt {/;
         push @{$self->buffer},
              "\$Polymake::Core::Application::extension->activate_configure_script; warn \"CONFIGURE_BUILD is not supported since polymake 2.13,\\nthe extension must be ported to the script-based configuration model\"; $header\n";
      } else {
         push @{$self->buffer},
              "BEGIN { die 'CONFIGURE_BUILD was formerly allowed in extensions only; it is not supported since polymake 2.13' }\n";
      }
   },

   object => sub { reopen_type(@_, "objects") },

   object_specialization => sub { reopen_specialization(@_) },

   property_type => sub { reopen_type(@_, "props") },

   default_object => sub {
      my ($self, $header)=@_;
      if ($header =~ /($type_re) \s*;$/xo) {
         push @{$self->buffer},
              "application::self()->default_type=typeof $1;\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid type name' }\n";
      }
   },

   property => sub {
      my ($self, $header)=@_;
      if ($header =~ /^ (?'prop_name' $id_re) \s*:\s* (?'prop_type' $type_re) \s* (?'attrs' (?: : [^:;{=]++ )*)
                                                  (?: (?'default' =\s*[^;]++)? ; | (?'open_scope' \{)) \s*$/xo) {

         my ($prop_name, $prop_type, $attrs, $default, $open_scope)=@+{qw(prop_name prop_type attrs default open_scope)};

         if ($Help::gather) {
            # always include the type in the displayed text
            append_comments($self, "# \@display property $prop_name : $prop_type\n");
            fill_help($self, "self(1)", "'properties', '$prop_name'");
         }
         my @attrs;
         my $flags=0;
         while ($attrs =~ s/:\s* (mutable|multiple|non_storable) \s* (?=:|$)//ox) {
            no strict 'refs';
            $flags |= ${"Polymake::Core::Property::is_$1"};
         }
         push @attrs, "flags=>$flags" if $flags;
         while ($attrs =~ s/:\s* (?:(canonical|equal)|(construct)) \s*\( ($balanced_re) \)\s*//ox) {
            push @attrs, $1 ? "$1=>$3" : "$2=>'$3'";
         }
         if ($attrs =~ /\S/) {
            push @{$self->buffer},
                 "BEGIN { die 'unknown property attributes $attrs' }\n";
            return;
         }
         ++$funcnt if defined($default);
         push @{$self->buffer},
              ($open_scope && "{ my \$prop=") .
              "self(1)->add_property_definition('$prop_name', (typeof $prop_type), @attrs); " .
              ( defined($default)
                ? "application::self()->add_default_value_rule('$prop_name : ', \\&__prod__$funcnt, self(1)); " .
                  "sub __prod__$funcnt : method { \$_[0]->$prop_name$default }"
                : ($open_scope &&
                   "  package _::_prop_$prop_name; \$prop->analyze(__PACKAGE__); undef \$prop;")
              ) . "\n";

      } elsif ($header =~ /^ ($id_re) \s*=\s* (override \s+)? ($id_re) \s*;\s*$/xo) {
         my ($prop_name, $override, $alias)=($1, 0+defined($2), $3);
         if ($Help::gather) {
            push @{$self->buffer},
                 "self(1)->help_topic(1)->add(['properties', '$prop_name'], <<'_#_#_#_' . '# \@display property $prop_name : '.(self(1)->help_topic(1)->find('properties', '$alias')->annex->{header} =~ / : (\$type_re)/)[0].\"\\n\");\n",
                 cut_comments($self),
                 "# Alias for property [[$alias]].\n",
                 "_#_#_#_\n",
                 "#line ".$self->header_line."\n";
         }
         push @{$self->buffer},
              "self(1)->add_property_alias('$prop_name', '$alias', $override);\n";

      } elsif ($header =~ /^($hier_id_re) \s*\{\s*$/xo) {
         my $prop_name=$1;
         (my $pkg=$prop_name) =~ s/(?:^|\.)/::_prop_/g;
         push @{$self->buffer},
              "{ self(1)->reopen_subobject('$prop_name'); package _$pkg;\n";

      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid property declaration' }\n";
      }
   },

   custom => sub {
      my ($self, $header)=@_;
      my ($varname, $tail);
      if ($header =~ /^[\$\@%] $id_re (?!:) (?= \s*(=))?/xo) {
         substr($header,0,0)="declare ";
         $varname=$&;
         $tail= (defined($1) ? "0" : "$. < \$__preamble_end && \$Core::Customize::state_config") . ", __PACKAGE__";
         push @{$self->buffer}, "#line ".$self->header_line."\n", "$header\n";
      } elsif ($header =~ /^[\$\@%] $qual_id_re (?= \s*(=))?/xo) {
         $varname=$&;
         push @{$self->buffer}, "$header\n" if defined($1);
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid custom variable name' }\n";
         return;
      }
      push @{$self->trailer},
           "application::self()->add_custom('$varname', <<'_#_#_#_', $tail);\n",
           cut_comments($self),
           "_#_#_#_\n";

      # must collect embedded comments in the hashes
      if ($header =~ /% $qual_id_re \s*=\s* \(/xo) {
         $self->filter=$self->custom_hash_filter;
      }
   },

   options => sub {
      my ($self, $header)=@_;
      if ($header =~ s/^%($qual_id_re)\s*//o) {
         my $varname=$1;
         push @{$self->buffer},
              $varname =~ /::($id_re)$/
              ? "%{namespaces::declare_var('$`', '%$1')}$header\n"
              : "declare %$varname$header\n";

         if ($Help::gather) {
            push @{$self->trailer},
                 "application::self()->help->add(['options', '$varname'], <<'_#_#_#_');\n",
                 cut_comments($self),
                 "_#_#_#_\n";
            $self->filter=$self->custom_hash_filter(1);
         }
      } else {
         push @{$self->buffer}, "BEGIN { die 'options must be declared as a hash variable' }\n";
      }
   },

   label => sub {
      my ($self, $header)=@_;
      if ($header =~ $id_only_re) {
         fill_help($self, "", "'preferences', '$1'") if $Help::gather;
         push @{$self->buffer},
              "application::self()->add_top_label('$1');\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid label name' }\n";
      }
   },

   prefer => sub {
      my ($self, $header)=@_;
      $header =~ s/\s+$//;
      push @{$self->buffer},
           "application::self()->prefs->add_preference('$header', 3);\n";
   },

   function => sub { prepare_function(@_, 0, "", "") },

   method => sub { prepare_function(@_, 1, "", "Core::check_application_pkg(0);") },

   user_function => sub { prepare_function(@_, 0, "u", !$_[0]->from_embedded_rules && "Core::check_application_pkg(1);") },

   user_method => sub { prepare_function(@_, 1, "u", "Core::check_application_pkg(0);") },

   global_method => sub { prepare_function(@_, 1, "g", "Core::check_application_pkg(0);") },

   type_method => sub { prepare_function(@_, 1, "t", "") },

   rule => sub {
      my ($self, $header)=@_;
      if ($header =~ s/^ ($hier_id_re \s*=\s* (?: $hier_id_re | \$this)) \s*;//xo) {
         my $rule_header=$1;
         $self->after_rule=1;
         push @{$self->buffer},
              "application::self()->add_production_rule('$rule_header', undef, self(1));\n";

      } elsif ($header =~ s/^ $hier_id_re (?: \s*[:|,]\s* $hier_id_re )* (?: \s*: )?//xo) {
         my $rule_header=$&;
         unless ($header =~ /\s*;\s*$/) {
            $self->filter=\&provide_rule_prologue;
            provide_rule_prologue($self,$header);
            $self->after_rule=1;
         };
         ++$funcnt;
         push @{$self->buffer},
              "application::self()->add_production_rule('$rule_header', \\&__prod__$funcnt, self(1), \$__cur_credit); sub __prod__$funcnt : method $header\n";

      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid rule header' }\n";
      }
   },

   auto_cast => sub {
      my ($self, $header)=@_;
      if ($header =~ /^: \s* ($type_re) \s*;\s* $/xo) {
         $self->after_rule=1;
         push @{$self->buffer},
              "self(1)->add_auto_cast(typeof $1);\n";

      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid auto_cast declaration' }\n";
      }
   },
);

sub inject_short_prologue {
   if ($_[1] =~ s/(?: ^ | \bsub \s+ $id_re) \s* \{ \K/$_[2]/xo) {
      undef $_[0]->filter;
   }
}

sub provide_rule_prologue : method {
   inject_short_prologue(@_, 'my $this=shift;');
}

my $display_credit=q{$__cur_credit->display if $__cur_credit && $Verbose::credits > $__cur_credit->shown;};

sub provide_short_prologue {
   my ($self, undef, $prologue)=@_;
   $self->filter=sub : method { inject_short_prologue(@_, $prologue) };
   inject_short_prologue($self, $_[1], $prologue);
}

sub inject_long_prologue {
   my ($self, $line, $prologue)=@_;
   if ($line =~ /^(\s*\{) \s*+ (.*) $/x) {
      if (length($2)) {
         push @{$self->buffer},
              "$1\n",
              @$prologue,
              "#line $.\n";
         $_[1]="$2\n";
      } else {
         push @{$self->buffer},
              "$1\n",
              @$prologue;
         $_[1]="#line ".($.+1)."\n";
      }
      undef $self->filter;
   }
}

sub provide_long_prologue {
   my ($self, $header, $prologue)=@_;
   if (defined $prologue) {
      if ($header =~ /^(\s*\{) \s*+ (.*) $/x) {
         $self->buffer->[-1].="$1\n";
         if (length($2)) {
            push @{$self->buffer},
                 @$prologue,
                 "#line $.\n",
                 "$2\n";
         } else {
            push @{$self->buffer},
                 @$prologue,
                 "#line ".($.+1)."\n";
         }
      } else {
         $self->filter=sub : method { inject_long_prologue(@_, $prologue) };
      }
   } else {
      $self->buffer->[-1].="$header\n";
   }
}

%decl_headers=(
   object => \&process_object_decl,

   object_specialization => \&process_object_specialization,

   property_type => \&process_property_type,

   permutation => \&process_permutation_decl,
);

my $cpp_init=<<'_#_#_#_';
self()->cpp->start_loading($Polymake::Core::Application::extension);
_#_#_#_

# embedded rules must be read in after all IMPORTs, otherwise the syntax parsing may fail
sub close_main_init_preamble {
   my $self=$_[0];
   push @{$self->buffer}, $cpp_init, "#line ".$self->header_line."\n";
   $self->rule_header_table=\%rule_headers;
   $self->decl_rule_header_table=\%decl_headers;
   $self->application->declared |= $main_init_closed;
}

sub deny_in_cross_app_scope {
   my $self=$_[0];
   push @{$self->buffer},
        "BEGIN { die 'This declaration is not allowed in a scope restricted by REQUIRE_APPLICATION' }\n";
}

while (my ($keyword, $code)=each %main_init_rule_headers) {
   $rule_headers{$keyword} //= $code;
}
while (my ($keyword, $code)=each %rule_headers) {
   $main_init_rule_headers{$keyword} //= sub { &close_main_init_preamble; &$code; };
   $cross_app_rule_headers{$keyword} = $keyword =~ /^(?:object|property_type|options|prefer|(?:user_)?function|(?:user_|global_)?method)$/
                                       ? $code : \&deny_in_cross_app_scope;
}
while (my ($keyword, $code)=each %decl_headers) {
   $main_init_decl_headers{$keyword} = sub { &close_main_init_preamble; &$code; };
   $cross_app_decl_headers{$keyword} = \&deny_in_cross_app_scope;
}


my %rule_subheaders=(
   precondition => sub {
      my ($self, $header)=@_;
      if ($header =~ /^ : (?: \s* (?: !\s*)? (?:exists|defined) \s*\(\s* $hier_id_alt_re \s*\)\s* (?: , | (?= ;)) )+ ;\s*$ /xo) {
         my $text="";
         while ($header =~ / (?:(!)\s*)? (?:(exists)|defined) \s*\(\s* ($hier_id_alt_re) \s*\)/gxo) {
            my $rule_header=$&;
            my ($not, $exists, $prop)=($1,$2,$3);
            if ($exists) {
               $text .= "application::self()->append_rule_existence_check('$not', '$prop', self(1));";
            } else {
               (my $expr=$prop) =~ s/\./->/g;
               $text .= "application::self()->append_rule_precondition(': $prop', sub { ${not}defined(\$_[0]->$expr) }, self(1), 1);";
            }
         }
         push @{$self->buffer}, "$text\n";

      } elsif ($header =~ s/^ :\s* (!\s* (\(\s*)?)? ($hier_id_alt_re) (?(2) (\s*\))) \s*;\s*$//xo) {
         my ($not, $prop, $not2)=($1, $3, $4);
         my $expr;
         if ($prop =~ /\|/) {
            $expr=join(" || ", map { "\$this->lookup('$_')" } split /\s*\|\s*/, $prop);
         } else {
            ($expr=$prop) =~ s/\./->/g;
            $expr='$this->'.$expr;
         }
         push @{$self->buffer},
              "application::self()->append_rule_precondition(': $prop', sub { my \$this=shift; $not$expr$not2 }, self(1), 0);\n";

      } elsif ($header =~ s/^ :\s* $hier_id_re (?: \s*[|,]\s* $hier_id_re )*//xo) {
         my $rule_header=$&;
         $self->filter=\&provide_rule_prologue;
         provide_rule_prologue($self,$header);
         ++$funcnt;
         push @{$self->buffer},
              "application::self()->append_rule_precondition('$rule_header', \\&__prec__$funcnt, self(1), 0); sub __prec__$funcnt : method $header\n";

      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid rule header' }\n";
         return;
      }
      $self->after_rule=1;
   },

   weight => sub {
      my ($self, $header)=@_;
      if ($header =~ s/^ (?: (\d+)\.(\d+) (\s*;)? )? \s* (?(3) $ | (:\s* $hier_id_re (?: \s*[|,]\s* $hier_id_re )*)?)//xo
          and defined($3) || defined($4)) {
         $self->after_rule=1;
         my $static= defined($1) ? "$1,$2" : "undef,undef";
         if (defined (my $rule_header=$4)) {
            $self->filter=\&provide_rule_prologue;
            provide_rule_prologue($self,$header);
            ++$funcnt;
            push @{$self->buffer},
                 "application::self()->append_rule_weight($static, '$rule_header', \\&__dynw__$funcnt, self(1)); sub __dynw__$funcnt : method $header\n";
         } else {
            push @{$self->buffer},
                 "application::self()->append_rule_weight($static);\n";
         }
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid rule header' }\n";
      }
   },

   override => sub {
      my ($self, $header)=@_;
      my $line="";
      my $good= $header =~ s/^:\s*(.*);\s*$/$1/;
      while ($header =~ s{^ (?'super' $type_qual_re) :: (?'label' $hier_id_re) \s* (?: ,\s* | $)}{}xo) {
         my ($super, $label)=@+{qw(super label)};
         if ($super eq "SUPER") {
            $super="'SUPER'";
         } else {
            $super="(typeof $super)";
         }
         $line.="application::self()->append_overridden_rule(self(1), $super, '$label'); ";
      }
      if ($good && $header !~ /\S/) {
         $self->after_rule=1;
         push @{$self->buffer}, $line."\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid rule header' }\n";
      }
   },

   permutation => sub {
      my ($self, $header)=@_;
      if ($header =~ s/^ :\s* ($id_re) \s*; $//xo) {
         $self->after_rule=1;
         push @{$self->buffer},
              "application::self()->append_rule_permutation('$1', self(1));\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid rule header' }\n";
      }
   },
);

#####################################################################################################
sub process_configure {
   my ($self, $header, $optional)=@_;
   ++$funcnt;
   $header =~ s/^\s*\{/sub __conf__$funcnt { use namespaces '+', 'Polymake::Configure';/;
   push @{$self->buffer},
        &start_preamble . "->configure(\\&__conf__$funcnt, '" . $self->rule_key . "', $., $optional) || return 1; $header\n";
}
#####################################################################################################
# recognize special configuration modes and modify the parsing routines correspondingly

sub pretend_configure_failed {
   my ($self, $header)=@_;
   push @{$self->buffer},
        &start_preamble . "->configure(sub { 0 }, '" . $self->rule_key . "', $.);\n",
        "1; __END__\n";
   close $self->handle;
}

sub ignore_optional_configure_clause {
   my ($self, $header)=@_;
   ++$funcnt;
   $header =~ s/^/sub __conf__$funcnt /;
   push @{$self->buffer}, "$header\n";
}

sub allow_config {
   my $mode=shift;
   if ($mode eq "none") {
      # pretend every configuration attempt has failed
      $rule_headers{CONFIGURE}=\&pretend_configure_failed;
      $main_init_rule_headers{CONFIGURE}=\&pretend_configure_failed;

      $rule_headers{CONFIGURE_OPT}=\&ignore_optional_configure_clause ;
      $main_init_rule_headers{CONFIGURE_OPT}=sub { &close_main_init_preamble; &ignore_optional_configure_clause };

      0  # don't load any config files

   } elsif ($mode eq "ignore") {
      # pretend everything is configured
      *Polymake::Core::Application::configure=sub { 1 };

      0  # don't load any config files

   } else {
      1  # allow loading config files
   }
}
#####################################################################################################
my $has_interactive_commands;

sub start_preamble {
   my $self=$_[0];
   my $start=!$self->preamble_end;
   $self->preamble_end=$.;
   "application::self()" . ($start && $has_interactive_commands && !$self->from_embedded_rules &&
                            "->start_preamble('".$self->rule_key."', Polymake::Core::rescue_static_code(0))")
}
#####################################################################################################
#  helper routines common for object and property type declaration processing

sub process_template_params($$\@\@\@\@$) {
   my ($text, $super_text, $param_names, $prologue, $super_abstract, $super_instance, $for_spez)=@_;

   my ($i, $defaults_seen)=(0,0);
   while ($text =~ m{\G $type_param_re \s* (?: ,\s* | $ ) }xog) {
      push @$param_names, $+{name};
      if (defined (my $default=$+{dynamic})) {
         translate_type_expr($default);
         push @$prologue,
              "  \$_[$#$param_names] //= $default;\n";
         ++$defaults_seen;
      } elsif (defined ($default=$+{static})) {
         push @$prologue,
              "  \$_[$#$param_names] //= typeof $default;\n";
         ++$defaults_seen;
      } elsif ($defaults_seen) {
         return "wrong order of parameters with and without default types";
      }
   }

   if ($for_spez && $defaults_seen) {
      return "type parameters can't have default values";
   }

   if (@$param_names>1) {
      push @$prologue,
           "  my \$type_inst=\\%type_inst;\n",
           "  foreach my \$arg (\@_[0.." . ($#$param_names-1) ."]) { \$type_inst=( \$type_inst->{\$arg} //= { } ) }\n",
           "  \$type_inst->{\$_[$#$param_names]}\n";
   } else {
      push @$prologue,
           "  \$type_inst{\$_[0]}\n",
   }

   if ($defaults_seen) {
      unshift @$prologue,
         $#$param_names >= $defaults_seen ?
         ( "  croak('too few type parameters for ', __PACKAGE__, ' : ', \$#_) if \$#_ < " . ($#$param_names - $defaults_seen) . ";\n" ) : (),
           "  croak('too many type parameters for ', __PACKAGE__, ' : ', \$#_) if \$#_ > $#$param_names;\n";
   } else {
      unshift @$prologue,
           "  croak('wrong number of type parameters for ', __PACKAGE__, ' : ', \$#_) if \$#_ != $#$param_names;\n";
   }

   my $needs_use_Params=$defaults_seen;

   while ($super_text =~ m{\G $type_expr_re \s*(?:,\s*)?}xgo) {
      if (defined (my $super_type=$+{dynamic})) {
         if ($for_spez) {
            return "dynamic type expression not allowed";
         }
         translate_type_expr($super_type);
         push @$super_instance, $super_type;
         $needs_use_Params=1;
      } else {
         $super_type=$+{static};
         my $super_expr="(typeof $super_type)";
         if (my ($super_abstract_type, $super_params)= $super_type =~ m{^($id_re)\s*<(.*)}o) {
            my $dependent;
            foreach (@$param_names) {
               $dependent= $super_params =~ m{\b$_\b} and last;
            }
            if ($dependent) {
               if ($for_spez) {
                  push @$super_abstract, $super_abstract_type;
                  push @$super_instance, $super_type;
               } else {
                  push @$super_abstract, "typeof_gen $super_abstract_type";
                  push @$super_instance, $super_expr;
               }
               $needs_use_Params=1;
               next;
            }
         }
         if ($for_spez) {
            return "`big' object type must depend on type parameters";
         }
         push @$super_abstract, $super_expr;
      }
   }

   unshift @$prologue, "  use namespaces::Params \\*_;\n" if $needs_use_Params;

   $defaults_seen;
}

sub process_enum {
   my %bag;
   local $_=shift;
   my $cnt=-1;
   while (pos($_) < length($_)) {
      if (/\G \s* (["'])? ($id_re) (?: \s*=\s* (\d+))? (?(1) \1) (?: , | \s+ | $)/xgco) {
         $bag{$2}= defined($3) ? ( $cnt=$3 ) : ++$cnt;
      } else {
         croak( "invalid enum declaration" );
      }
   }
   bless \%bag, "Polymake::Enum";
}
#####################################################################################################

my @ellipsis_prologue=split /(?<=\n)/, <<'.';
  my $type_inst=my $root=\%type_inst;
  $type_inst=( $type_inst->{$_} //= { } ) for @_;
  $type_inst->{$root}
.

sub check_outer_pkg {
   my ($self, $what)=@_;
   my $outer_pkg=compiling_in();
   if ($outer_pkg ne $self->application->pkg) {
      if ($plausibility_checks && index($outer_pkg, $self->application->pkg."::")==0) {
         my $msg= $what eq "object"
                  ? "a `big' object type must be declared" :
                  $what eq "property"
                  ? "a property type must be declared" :
                  $what eq "spez"
                  ? "a `big' type specialization must be declared" :
                  $what eq "objects"
                  ? "a `big' object type scope must be directly contained" :
                  $what eq "props"
                  ? "a property type scope must be directly contained" :
                  $what eq "spezs"
                  ? "a `big' type specialization scope must be directly contained"
                  : "??? internal error ???";
         push @{$self->buffer},
              "BEGIN { die '$msg in the top-level (application) scope' }\n";
         return;
      }
      "package application; "
   } else {
      ""
   }
}

sub generate_scope_type_params {
   "sub __scope_type_params { (is_object(\$_[0]) ? \$_[0] : \$_[0]->[0])->type->descend_to_generic(__PACKAGE__)->params } " .
   "use namespaces::Params *__scope_type_params, qw(@_);\n"
}

sub announce_parametrized_class {
   my ($self, $name, $subpkg, @param_names)=@_;
   get_pkg($name, 1);                                  # enforce a top-level package to fool the perl parser
   get_pkg($self->application->name."::$name", 1);     # and the same for qualifications via application name
   get_pkg($subpkg."::$name", 1);                      # and the same for qualifications via type family
   get_pkg($_, 1) for @param_names;                    # type parameter names must also be known as packages
}
#####################################################################################################
sub reopen_type {
   my ($self, $header, $where)=@_;
   if (my ($type)= $header =~ /^$type_re \s*\{\s*$/xo) {
      my $preamble=check_outer_pkg($self, $where);
      return unless defined($preamble);
      my ($outer_pkg, $check_app_pkg);
      my $sanity_check="";
      if ($type =~ s/^($id_re):://o) {
         if ($plausibility_checks) {
            $sanity_check="Core::check_proper_app_use(application::self(), '$1'); ";
         }
         $outer_pkg="Polymake::$1";
         $check_app_pkg="\\%Polymake::, '$1'";
      } else {
         $outer_pkg="_";
         $check_app_pkg='\\%application::';
      }

      if ($type =~ /</) {
         # shouldn't create prototype objects in a BEGIN block
         (my $pkg=$type) =~ s/\s//g;
         while ($pkg =~ s/($id_re)<($ids_re)>/ PropertyParamedType::mangle_paramed_type_name($1, $2) /goe) {}
         if ($plausibility_checks) {
            $type =~ /^$id_re/o;
            $sanity_check="BEGIN { $sanity_check Core::check_type_definition(1, $check_app_pkg, '$where', '$&') } ";
         }
         $preamble .= $sanity_check . "{ package ${outer_pkg}::$pkg; sub self { &Core::check_object_pkg; typeof $type }";
      } else {
         if ($plausibility_checks) {
            $sanity_check="BEGIN { $sanity_check Core::check_type_definition(1, $check_app_pkg, '$where', '$type') } ";
         }
         $preamble .= $sanity_check . "{ package ${outer_pkg}::$type;"
                      . " local_array(*__scope_type_params, typeof_gen(undef)->params) if typeof_gen(undef)->abstract;  use namespaces::Params *__scope_type_params;";
      }
      push @{$self->buffer}, "$preamble\n";
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid " . ($where eq "objects" ? "object" : "property") . " type reference' }\n";
   }
}
#####################################################################################################
sub reopen_specialization {
   my ($self, $header)=@_;
   if (my ($obj_type, $alias_name)= $header =~ /^($id_re)::($id_re) \s*\{\s*$/xo) {
      my $preamble=check_outer_pkg($self, "spezs");
      return unless defined($preamble);
      my $sanity_check=$plausibility_checks ? "BEGIN { Core::check_type_definition(1, \\%application::, 'objects', '$obj_type', '$alias_name') } " : "";
      push @{$self->buffer},
           $preamble . $sanity_check . "{ package _::$obj_type\::$alias_name;" .
           " local_array(*__scope_type_params, typeof_gen(undef)->params);  use namespaces::Params *__scope_type_params;\n";
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid type specialization reference' }\n";
   }
}
#####################################################################################################
sub process_property_type {
   my ($self, $header)=@_;
   my $first_line=@{$self->buffer};
   my $preamble=check_outer_pkg($self, "property");
   return unless defined($preamble);

   if ($header =~ /^ ($id_re) \s*=\s* ($type_re) \s*;\s* $/xo) {
      my ($type_name, $alias)=@_;
      if ($plausibility_checks) {
         $preamble .= "BEGIN { Core::multiple_prop_definition('$type_name') } ";
      }
      push @{$self->buffer},
           $preamble."{ my \$stash=get_pkg((typeof $alias)->pkg); *application::$type_name\::=\$stash; *application::props::$type_name\::=\$stash; }\n";

   } elsif ($header =~ /^ (?'type_name' $id_re) (?: $type_params_re | \s*<\s* (?'tparams' \.\.\. | \*) \s*> )?+
                                                (?: \s*:\s* (?!c\+\+)(?!upgrades)(?'super' $type_expr_re) )?+
                                                (?: \s*:\s* upgrades \s*\(\s* (?'upgrades' $types_re) \s*\) )?+
                                                (?: \s*:\s* (?'cpp_binding' c\+\+) (?: \s*\( (?'cpp_opts' $balanced_re) \) )?+)?+
                          \s* (?: (?'open_scope' \{ ) | ; ) \s*$/xo) {

      my ($type_name, $tparams, $typecheck, $super, $upgrades, $cpp_binding, $cpp_opts, $attr_name, $attr_value, $open_scope)=@+{qw
         ( type_name   tparams  typecheck    super   upgrades   cpp_binding   cpp_opts   attr_name   attr_value   open_scope)   };

      fill_help($self, "", "'property_types', '$type_name'") if $Help::gather;

      if ($plausibility_checks) {
         $preamble .= "BEGIN { Core::multiple_prop_definition('$type_name') } ";
      }
      $preamble .= "{ package _::$type_name; BEGIN { *application::props::$type_name\::=get_pkg(__PACKAGE__) } namespaces::memorize_lexical_scope;\n";
      push @{$self->buffer}, $preamble;

      my $buffer_size;
      if ($accurate_linenumbers && !$self->from_embedded_rules) {
         $buffer_size=@{$self->buffer};
         push @{$self->buffer},
              "#line ".$self->injected_lines." \"".$self->injected_source."\"\n";
      }

      translate_type_expr($upgrades) if defined($upgrades);

      if (defined($tparams)) {
         # parameterized type template

         my (@param_names, @prologue, @super_abstract, @super_instance, $n_defaults);
         if ($tparams eq "...") {
            if ($cpp_binding) {
               $self->buffer->[$first_line]="BEGIN { die 'type with arbitrary many parameters cannot have C++ binding' }\n";
               return;
            }
            if (defined($super)) {
               $self->buffer->[$first_line]="BEGIN { die 'type with arbitrary many parameters may not be derived' }\n";
               return;
            }
            @prologue=@ellipsis_prologue;
            $n_defaults=0;
         } elsif ($tparams eq "*") {
            if (defined($super)) {
               if ($super =~ /^[{(]/) {
                  $self->buffer->[$first_line]="BEGIN { die 'pure C++ types may not have a dynamic base class' }\n";
                  return;
               } elsif ($super =~ /</) {
                  push @super_abstract, "(typeof $super)";
               } else {
                  push @super_abstract, "typeof_gen $super";
               }
            }
            push @prologue, "  \$type_inst{\$_[0]}\n";
         } else {
            $n_defaults=process_template_params($tparams, $super, @param_names, @prologue, @super_abstract, @super_instance, 0);
            if (is_string($n_defaults)) {
               $self->buffer->[$first_line]="BEGIN { die 'invalid property type declaration: $n_defaults' }\n";
               return;
            }
            push @{$self->buffer}, generate_scope_type_params(@param_names);
            translate_type_expr($typecheck) if defined($typecheck);
         }

         push @{$self->buffer},
              "using namespaces 'Polymake::Core::PropertyParamedType';\n",
              "sub typeof_gen { state \$abstract_inst=" . ($cpp_binding && "application::self()->cpp->add_type_template(") .
              ($tparams eq "*"
               ? "new_generic Polymake::Core::PropertyTypeInstance('$type_name', __PACKAGE__, application::self(), @super_abstract)"
               : "new_generic Polymake::Core::PropertyParamedType('$type_name', __PACKAGE__, application::self(), [$n_defaults, qw(@param_names)], @super_abstract)") .
              ($cpp_binding &&
               ", template_params=>" . ($tparams ne "*" ? scalar(@param_names) : "'*'") . ",$cpp_opts)") .
              "; }\n";

         if ($tparams ne "*") {
            announce_parametrized_class($self, $type_name, "props", @param_names);

            push @{$self->buffer},
               "sub self { &Core::check_object_pkg; &typeof_gen }\n";
         }

         my $super_arg= @super_instance
                        ? "do { local_incr(\$Polymake::Core::PropertyType::nesting_level); @super_instance }"
                        : "undef";

         push @{$self->buffer},
              "sub typeof { shift; state %type_inst;\n",
              @prologue,
              "    //= do { my \$gen=&typeof_gen; " . (defined($typecheck) && "$typecheck;") . "\n",
              "         " .
              ($upgrades &&
               "(") .
              ($cpp_binding &&
               "\$gen->application->cpp->add_template_instance(") .
              ($tparams eq "*"
               ? "new Polymake::Core::PropertyTypeInstance(\$gen, $super_arg, \@_)"
               : "new Polymake::Core::PropertyParamedType(\$gen, $super_arg, \\\@_)" ) .
              ($cpp_binding &&
               ", \$gen, \$Polymake::Core::PropertyType::nesting_level)") .
              ($upgrades &&
               ")->add_upgrade_relations($upgrades)") .
              " } }\n",
              $open_scope
              ? (@param_names
                 ? "local_array(*__scope_type_params, &typeof_gen->params);\n" : ())
              : "typeof_gen() }\n";

      } else {
           # non-parameterized type
           if ($cpp_binding) {
              $cpp_opts =~ s/enum\s*(?=[({])($confined_re)/Polymake::Core::Application::RuleFilter::process_enum(q$1)/o;
              $cpp_opts =~ s/\bembedded\b/descr=>'embedded'/;
           }
           $super &&= "typeof $super";

           push @{$self->buffer},
                "sub typeof { \@==1 or croak('type $type_name is not parameterized');\n",
                "             state \$type_inst = new Polymake::Core::PropertyType('$type_name', __PACKAGE__, application::self(), $super); }\n",
                "*typeof_gen=\\&typeof;\n",
                "sub self { &Core::check_object_pkg; typeof(undef); }\n",
                $cpp_binding
                ? "application::self()->cpp->add_type(typeof(undef), $cpp_opts);\n" : (),
                $upgrades
                ? "typeof(undef)->add_upgrade_relations($upgrades);\n" : (),
                !$open_scope
                ? "}\n" : ();
      }

      if ($accurate_linenumbers && !$self->from_embedded_rules) {
         $self->injected_lines += @{$self->buffer}-$buffer_size;
         push @{$self->buffer}, "#line ".($.+1)." \"".$self->path."\"\n";
      } else {
         push @{$self->buffer}, "#line ".($.+1)."\n";
      }
      $self->prolonged=0;

   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid property type declaration' }\n";
   }
}
#####################################################################################################
sub process_object_decl {
   my ($self, $header)=@_;
   my $first_line=@{$self->buffer};
   my $preamble=check_outer_pkg($self, "object");
   return unless defined($preamble);

   if ($header =~ /^ $paramed_decl_re (?: (?: \s*:\s* (?'super' $type_exprs_re))?+ \s*(?: ; | (?'open_scope' \{))
                                        | (?('tparams') | \s*=\s* (?'alias' $type_re) (?: \s*;)?)) \s*$/xo) {

      my ($type_name, $tparams, $typecheck, $super, $open_scope, $alias)=@+{qw(lead_name tparams typecheck super open_scope alias)};

      if ($plausibility_checks) {
         $preamble .= "BEGIN { Core::multiple_object_definition('$type_name') } ";
      }

      if (defined($alias)) {
         push @{$self->buffer},
              $preamble."{ my \$stash=get_pkg((typeof $alias)->pkg); *application::$type_name\::=\$stash; *application::objects::$type_name\::=\$stash; }\n";

      } else {
         # defining a new object type
         fill_help($self, "", "'objects', '$type_name'") if $Help::gather;

         my $buffer_size;
         if ($accurate_linenumbers) {
            $buffer_size=@{$self->buffer};
            push @{$self->buffer},
                 "#line ".$self->injected_lines." \"".$self->injected_source."\"\n";
         }

         push @{$self->buffer},
              $preamble."{ package _::$type_name; BEGIN { *application::objects::$type_name\::=get_pkg(__PACKAGE__); } namespaces::memorize_lexical_scope;\n";

         if (defined($tparams)) {
            # parameterized template

            my (@param_names, @prologue, @super_abstract, @super_instance);
            my $n_defaults=process_template_params($tparams, $super, @param_names, @prologue, @super_abstract, @super_instance, 0);
            if (is_string($n_defaults)) {
               $self->buffer->[$first_line]="BEGIN { die 'invalid object declaration: $n_defaults' }\n";
               return;
            }
            translate_type_expr($typecheck) if defined($typecheck);

            push @{$self->buffer},
                 generate_scope_type_params(@param_names),
                 "sub typeof_gen { state \$abstract_inst=\n",
                 "  new Polymake::Core::ObjectType('$type_name', application::self(), [$n_defaults, qw(@param_names)], " . join(",", @super_abstract). "); }\n",
                 "sub self { &Core::check_object_pkg; &typeof_gen }\n",
                 "sub typeof { shift; state %type_inst;\n",
                 @prologue,
                 "    //= " . (defined($typecheck) && "do { $typecheck; ")
                            . "new Polymake::Core::ObjectType('$type_name', undef, \\\@_, &typeof_gen, " . join(",", @super_instance) . ");"
                            . (defined($typecheck) && " } ") . "}\n",
                 $n_defaults == @param_names
                 ? "application::self()->default_type //= typeof(undef);\n" : (),
                 $open_scope
                 ? "local_array(*__scope_type_params, &typeof_gen->params);\n"
                 : "typeof_gen(); }\n";

            announce_parametrized_class($self, $type_name, "objects", @param_names);

         } else {
            # non-parameterized type
            $super &&= "typeof $super";

            push @{$self->buffer},
                 "sub typeof { \@_==1 or croak('type $type_name is not parameterized');\n",
                 "             state \$type_inst=new Polymake::Core::ObjectType('$type_name', application::self(), undef, $super); }\n",
                 "*typeof_gen=\\&typeof;\n",
                 "sub self { &Core::check_object_pkg; typeof(undef) }\n",
                 "application::self()->default_type //= typeof(undef);\n",
                 !$open_scope
                 ? "typeof(undef); }\n" : ();
         }

         if ($accurate_linenumbers) {
            $self->injected_lines += @{$self->buffer}-$buffer_size;
            push @{$self->buffer}, "#line ".($.+1)." \"".$self->path."\"\n";
         } else {
            push @{$self->buffer}, "#line ".($.+1)."\n";
         }
      }
      $self->prolonged=0;
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid object declaration' }\n";
   }
}
#####################################################################################################
sub process_object_specialization {
   my ($self, $header)=@_;
   my $first_line=@{$self->buffer};
   my $preamble=check_outer_pkg($self, "spez");
   return unless defined($preamble);

   if ($header =~ /^ (?'spez_name' $id_re)? \s*<\s* (?'tparams' $ids_re) \s*>\s*=\s*
                     (?'obj_type' $type_expr_re) (?: \s*\[ (?'typecheck' $balanced_re) \s*\] )? \s*\{ /xo) {

      my ($spez_name, $tparams, $typecheck, $obj_type)=@+{qw(spez_name tparams typecheck obj_type)};
      my $pkg_name=$spez_name;
      my $visible_spez_name=$spez_name;

      my (@param_names, @prologue, @super_abstract, @super_instance);
      my $error=process_template_params($tparams, $obj_type, @param_names, @prologue, @super_abstract, @super_instance, 1);
      if (is_string($error)) {
         $self->buffer->[$first_line]="BEGIN { die 'invalid type specialization declaration: $error' }\n";
         return;
      }
      my $generic_type=$super_abstract[0];
      my $concrete_type=$super_instance[0];

      if (defined $spez_name) {
         $spez_name="$generic_type\::$spez_name";
         fill_help($self, "", "'objects', '$generic_type', 'specializations', '$spez_name'") if $Help::gather;
      } else {
         $spez_name="$generic_type<$tparams>";
      }

      my $buffer_size;
      if ($accurate_linenumbers) {
         $buffer_size=@{$self->buffer};
         push @{$self->buffer},
              "#line ".$self->injected_lines." \"".$self->injected_source."\"\n";
      }

      if (defined $pkg_name) {
         if ($plausibility_checks) {
            $preamble .= "BEGIN { Core::multiple_object_definition('$generic_type', '$pkg_name') } ";
         }
      } else {
         state $spez_cnt=0;
         $pkg_name="_Spez_".++$spez_cnt;
      }

      push @{$self->buffer},
           $preamble."{ package _::$generic_type\::$pkg_name; namespaces::memorize_lexical_scope;\n";

      push @{$self->buffer},
           generate_scope_type_params(@param_names),
           "sub typeof_gen { state \$abstract_inst=\n",
           "  new Polymake::Core::ObjectType::Specialization('$spez_name', __PACKAGE__, (typeof_gen $generic_type), [qw(@param_names)]); }\n",
           "sub self { &Core::check_object_pkg; &typeof_gen }\n",
           "sub typeof { shift; state %type_inst;\n",
           @prologue,
           "    //= new Polymake::Core::ObjectType::Specialization('$spez_name', undef, &typeof_gen, \\\@_); }\n",
           "local_array(*__scope_type_params, &typeof_gen->params);\n";

      my $match_func_name="_match_type";
      my $unique_name=$match_func_name."__inst";
      add_overloaded_instance($self,
                              "{ (typeof ".$self->application->pkg."::$generic_type\::$pkg_name<$tparams>) } " .
                              "&typeof_gen->apply_to_existing_types;",
                              undef, "", "",
                              $match_func_name, 0, $unique_name, "\\&$unique_name", $concrete_type, $tparams,
                              $typecheck, "root_node=>&typeof_gen->match_node");

      # reduced form of announce_parametrized_class
      get_pkg($visible_spez_name, 1) if defined($visible_spez_name);
      get_pkg($_, 1) for $tparams;

      if ($accurate_linenumbers) {
         $self->injected_lines += @{$self->buffer}-$buffer_size;
         push @{$self->buffer}, "#line ".($.+1)." \"".$self->path."\"\n";
      }
      $self->prolonged=0;
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid type specialization declaration' }\n";
   }
}
#####################################################################################################
sub process_permutation_decl {
   my ($self, $header)=@_;
   if (my ($name)= $header =~ /^($id_re) \s*\{\s*$/xo) {
      push @{$self->buffer},
           "self(1)->add_permutation('$name'); { package _::_prop_$name;\n";
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid permutation declaration' }\n";
   }
}
#####################################################################################################
sub prepare_context_check {
   my ($self, $func_name, $type_params, $context_check)=@_;
   if (defined $type_params) {
      if ($self->from_embedded_rules) {
         push @{$self->buffer_phase1}, "sub $func_name; namespaces::export_sub(undef, \\&$func_name);\n";
      } else {
         return "sub $func_name; BEGIN { namespaces::export_sub(undef, \\&$func_name); $context_check }";
      }
   }
   $context_check && "BEGIN { $context_check }";
}
#####################################################################################################
sub add_overloaded_instance {
   my ($self, $header, $context_check, $user, $global,
       $func_name, $method, $unique_name, $subref, $signature, $type_params,
       $typecheck_code, $opts, $cxx_func_attrs, $cxx_options)=@_;
   my (@arg_list, $min, $complex_defaults, $type_deduction);
   my (@default_types, @process_kw, @type_param_names, @type_param_mandatory, @default_values, @process_default_values, @errors);
   my (@cxx_arg_attrs);
   my $cxx=defined($cxx_func_attrs);
   my $max=0;

   if ($cxx && $method) {
      substr($cxx_func_attrs, 0, 0).="method=>1,";
   }
   if ($signature =~ s/(?: ^ | [,;]\s* | \s+) ((?: \\?% | \{ ) .*)//x) {
      $max |= $Overload::has_keywords;
      my $kw_tables=$1;
      if ($kw_tables =~ /%\s*$/) {
         push @process_kw, "  Overload::process_kw_args(\\\@_);\n";
      } else {
         $kw_tables =~ s/(?<! \\) \s* %(?=$qual_id_re)/\\%/gxo;   # pass hash references, not copies
         push @process_kw, "  Overload::process_kw_args(\\\@_, $kw_tables);\n";
      }
   }

   if (defined $type_params) {
      $type_params =~ s/^\s+//;
      while ($type_params =~ m{\G $type_param_re \s* (?: ,\s* | $ ) }xog) {
         my ($name, $default_type)=@+{qw(name default)};
         push @type_param_names, $name;
         if (defined $default_type) {
            translate_type_expr($default_type);
            $default_types[$#type_param_names]=$default_type;
            push @type_param_mandatory, 0;
         } else {
            push @type_param_mandatory, 1;
         }
      }
      create FunctionTypeParam(scalar(@type_param_names));
      if ($cxx) {
         $cxx_options .= "," if length($cxx_options);
         $cxx_options .= "explicit_template_params=>" . scalar(@type_param_names);
      }
   }

   $signature =~ s/^\s+//;
   while (pos($signature) < length($signature)) {
      if ($signature =~ /\G ; \s*/gxc) {
         if (defined $min) {
            push @errors, "invalid signature: multiple ';'";
            last;
         }
         $min=@arg_list;

      } elsif ($signature =~ /\G (?: \$ | (?: (?'star' \*) | (?'type' $type_re)) (?'attrs' (?: \s*:\s* $id_re)+ | &)?)
                                 (?('star') | (?('attrs') | (?: \s*=\s* (?'default' $expression_re) | (?('type') \s*(?'repeated' \+))? ))) \s*(?:,\s*)?/gxco) {
         my ($star, $type, $attrs, $default_value, $repeated)=@+{qw(star type attrs default repeated)};

         if ($cxx) {
            if ($attrs eq "&") {
               if (!$self->from_embedded_rules) {
                  push @errors, "Reference sign '&' is only allowed in C++ clients; in rule files an attribute ':lvalue' or ':lvalue_opt' must be used instead.";
                  last;
               }
               $attrs=":lvalue";
            }
            push @cxx_arg_attrs, "'$star$attrs'";
         } else {
            if ($star) {
               push @errors, "type wildcard '*' is only allowed in C++ function declarations";
               last;
            }
            if ($attrs) {
               push @errors, "argument attributes are only allowed in C++ function declarations";
               last;
            }
         }

         my $arg_index=@arg_list;
         if (defined $type) {
            if (@type_param_names) {
               while ($type =~ /$qual_id_re/go) {
                  if ($type eq "_") {
                     $type_deduction=1;
                  } else {
                     my $tp_index=string_list_index(\@type_param_names, $&);
                     if ($tp_index>=0) {
                        # If the type parameter is involved in a final typecheck, we can safely assume that
                        # it will either be set or the typecheck will fail if no optional argument suitable for type deduction has been passed.
                        $type_param_mandatory[$tp_index] &&= defined($min) && $typecheck_code !~ /\b$type_param_names[$tp_index]\b/;
                        $type_deduction=1;
                     }
                  }
               }
            }
            my $typeof= $type =~ /</ ? "typeof" : "typeof_gen";
            if ($repeated) {
               $max |= $Overload::has_repeated;
               push @arg_list, "[ ($typeof $type), '+' ]";
            } else {
               push @arg_list, "($typeof $type)";
            }
         } else {
            push @arg_list, "'\$'";
         }
         if (defined $default_value) {
            unless (defined $min) {
               push @errors, "only optional arguments may have default values";
               last;
            }
            $complex_defaults += $default_value =~ s/^(?=\s*\{.*\}\s*$)/do /;
            $complex_defaults ||= $default_value =~ /[()<>]|\$_|\@_/;
            $default_values[$arg_index-$min]=$default_value;
         } elsif (defined($min) && $cxx) {
            push @errors, "optional argument without default value not allowed in a C++ function";
            last;
         }

      } elsif ($signature =~ /\G \@ \s*$/gxc) {
         if ($max & $Overload::has_keywords) {
            push @errors, "unlimited trailing argument list is not compatible with keyword arguments";
            last;
         } else {
            $max |= $Overload::has_trailing_list;
         }
      } else {
         push @errors, "invalid function signature, parser stopped at `!' : '" . substr($signature, 0, pos($signature)) . "!" . substr($signature, pos($signature)) . "'";
         last;
      }
   }

   $min //= @arg_list;
   $max += @arg_list;

   if (@type_param_names) {
      my $tp_min=0;
      while ($tp_min < @type_param_names && $type_param_mandatory[$tp_min]) { ++$tp_min }
      my $tp_max=$tp_min;
      while ($tp_max < @type_param_names) {
         if ($type_param_mandatory[$tp_max]) {
            push @errors, "type parameter $type_param_names[$tp_max] lacks a default value and can't be deduced from mandatory arguments",
                          "but follows others with defaults or deducible ones";
            last;
         }
         ++$tp_max;
      }
      $opts .= "," if $opts;
      $opts .= "tparams=>[$tp_min,$tp_max]";
   }

   if (@errors) {
      push @{$self->buffer},
           'BEGIN { die( join("\\n", ' . join(", ", map { "q{$_}" } @errors) . ") ) }\n";
      return;
   }

   if ($complex_defaults) {
      my $skipped_undef_values;
      for (my $i=$min; $i<@arg_list; ++$i) {
         if (defined (my $default_value=$default_values[$i-$min])) {
            my $arg_index=$i+$method;
            push @process_default_values,
                 "  if (\@_ <= $arg_index) { push \@_, " .
                 ($skipped_undef_values && "(undef)x($arg_index-@_), ") .
                 "$default_value; }\n";
            $skipped_undef_values="";
         } else {
            $skipped_undef_values=1;
         }
      }
      if ($max & $Overload::has_keywords and $skipped_undef_values) {
         # keyword tables must be appended at the expected position at the end of the argument list
         my $last_pos_arg=$#arg_list+$method;
         push @process_default_values,
              "  if (\$#_ < $last_pos_arg) { \$#_=$last_pos_arg }\n";
      }

   } elsif ($min < @arg_list) {
      my $min_arg_index=$min+$method;
      if (@default_values) {
         if ($max & $Overload::has_keywords) {
            $#default_values=@arg_list-$min-1;
         }
         push @process_default_values,
              "  state \$__default_values=[" . join(", ", map { defined($_) ? $_ : "undef" } @default_values) . "];\n",
              "  push \@_, \@\$__default_values[\@_-$min_arg_index..$#default_values];\n";

      } elsif ($max & $Overload::has_keywords) {
         my $last_pos_arg=$#arg_list+$method;
         push @process_default_values,
              "  if (\$#_ < $last_pos_arg) { \$#_=$last_pos_arg }\n";
      }
   }

   if ($typecheck_code) {
      my $final_typecheck="${unique_name}_tpck";
      translate_type_expr($typecheck_code);

      push @{$self->buffer},
           "sub $final_typecheck {\n",
           "  my (\$__typelist, \$__explicit_size)=namespaces::fetch_explicit_typelist(\$_[0]);\n",
           "  use namespaces::Params \$__typelist, qw(@type_param_names);\n";

      if (@default_types || $typecheck_code =~ /type_upgrade/) {
         push @{$self->buffer},
              "  my \$__lasttype=\$#\$__typelist;\n",
              "  push \@{\$_[1]}, \@\$__typelist[\$__explicit_size..\$__lasttype], \$__lasttype, \\&Overload::restore_type_param_list;\n";

         for (my $i=0; $i<=$#default_types; ++$i) {
            if (defined $default_types[$i]) {
               push @{$self->buffer},
                    "  \$__typelist->[$i] //= $default_types[$i];\n";
            }
         }
         $#default_types=-1;
      }

      push @{$self->buffer},
           "  $typecheck_code }\n";

      push @arg_list, "\\&$final_typecheck";
   }

   my @preamble;
   if (@type_param_names) {
      push @preamble, "  use namespaces::Params qw(@type_param_names);\n";
   }
   if (@default_types) {
      push @preamble,
           "  { my \$__typelist=namespaces::fetch_explicit_typelist(\\\@_);\n";

      for (my $i=0; $i<=$#default_types; ++$i) {
         if (defined $default_types[$i]) {
            push @preamble,
                 "  \$__typelist->[$i] //= $default_types[$i];\n";
         }
      }
      push @preamble,
           "  }\n";
   }
   push @preamble, "  $display_credit\n" if ($user || $global) && $self->credit_seen;
   push @preamble, @process_default_values, @process_kw;

   my $deferred_preamble;
   my $orig_subref=$subref;
   if (@preamble) {
      if (length($subref)) {
         # the preamble is inserted into the transformed code
         $deferred_preamble=\@preamble;
      } elsif ($cxx && @preamble==1 && @type_param_names) {
         # a preamble for pure C++ functions should only be generated if it contains some actions
         $subref="undef";
         @preamble=();
      } else {
         $subref="\\&${unique_name}_preamble";
      }
   } else {
      $subref ||= "undef";
   }

   my $ov_line=defined($context_check) ? prepare_context_check($self, $func_name, $type_params, $context_check) : "";

   if ($type_deduction) {
      $ov_line .= " { use namespaces::Params \\*Polymake::Core::FunctionTypeParam::instances, qw(_ @type_param_names);";
   }

   if (defined $signature) {
      $ov_line .= ($func_name eq "construct"
                   ? " self(1)->add_constructor("
                   : " add$global Overload(") .
                  ($cxx && "application::self()->cpp->add_auto_function(") .
                  "'$func_name', $subref, [ $min, $max, " . join(", ", @arg_list) . " ]," .
                  ($cxx && "[" . join(", ", @cxx_arg_attrs) . "], { $cxx_func_attrs }, { $cxx_options }),") .
                  "$opts)";
   } else {
      $cxx or croak( "internal error: pure perl function went into a wrong throat" );
      $cxx="application::self()->cpp->add_auto_function('$func_name', $subref, undef, undef, { $cxx_func_attrs }, { $cxx_options })";
      if ($opts) {
         $ov_line .= " add$global Overload($cxx, $opts)";
      } else {
         $ov_line .= " $cxx";
      }
   }

   if ($type_deduction) {
      $ov_line .= " }";
   }

   if (@preamble && !defined($deferred_preamble)) {
      push @{$self->buffer},
           "sub ${unique_name}_preamble {\n",
           @preamble,
           "}\n",
           "#line ".$self->header_line."\n";
   }

   if (length($orig_subref)) {
      $ov_line .= "; sub $unique_name";
      if ($method && !$cxx) {
         $ov_line .= " : method";
      }
   }
   push @{$self->buffer}, $ov_line;
   provide_long_prologue($self, $header, $deferred_preamble);
   1
}
#####################################################################################################
sub prepare_function {
   my ($self, $header, $method, $kind, $context_check)=@_;
   my ($user, $global, $type_method)=($kind eq "u", $kind eq "g" && "_global", $kind eq "t");
   if ($header =~ $labeled_sub_re) {
      my ($labels, $name, $type_params, $signature, $typecheck)=@+{qw(labels lead_name tparams signature typecheck)};
      $header=$';

      if ($user) {
         fill_help($self,
                   $method ? ("self(-1)", "'methods', '$name'")
                           : ("", "'functions', '$name'"),
                   "q($signature)") if $Help::gather;
         $self->application->EXPORT->{$name} ||= $method ? "meth" : "user";
      }
      if ($plausibility_checks && $name eq "construct") {
         if (!defined($signature) || !$method || $kind) {
            push @{$self->buffer},
                 "BEGIN { die '\\'construct\\' must be defined as an overloaded method' }\n";
            return;
         }
      }
      my $meth_decl= $method ? " : method" : "";
      if ($header =~ s/^[^\#]*? \K :\s* c\+\+ \s* (?: \(\s* ($balanced_re) \s*\)\s* )? (?=[:;\{])//xo) {
         my $options=$1;
         if ($plausibility_checks) {
            if ($type_method) {
               push @{$self->buffer},
                    "BEGIN { die 'type_method can\'t have a C++ binding' }\n";
               return;
            }
            if (!defined($signature)) {
               $context_check.=" exists &$name and Core::multiple_func_definition();";
            }
         } else {
            $context_check="";
         }
         my @attrs;
         while ($header =~ s/^([^\#]*?) :\s* (\w+) \s* (?: \(\s* ($balanced_re) \s*\)\s*)? (?=[:;\{])/$1/x) {
            push @attrs, "$2=>".( defined($3) ? "'$3'" : 1);
         }
         if ($header =~ /^\s* (?: ;\s* (?: \#.*)? $ | (\{))/x) {
            ++$funcnt;
            my $unique_name="__${name}__OV__$funcnt";
            my $subref=$1 && "\\&$unique_name";
            $labels &&= "label=>[ application::self()->add_labels('$labels') ]";
            add_overloaded_instance($self, $header, $context_check, $user, $global,
                                    $name, $method, $unique_name, $subref, $signature, $type_params,
                                    $typecheck, $labels, join(",", @attrs), $options)
              or return;
         } else {
            push @{$self->buffer}, "BEGIN { die 'unknown attributes for C++ " . ($method ? "method" : "function") . "' }\n";
         }

      } elsif (defined($signature) || defined($labels)) {
         if ($plausibility_checks) {
            if ($type_method) {
               push @{$self->buffer},
                    "BEGIN { die 'type_method can\'t be overloaded or labeled' }\n";
               return;
            }
         } else {
            $context_check="";
         }
         my @opts;
         ++$funcnt;
         my $unique_name="__${name}__OV__$funcnt";
         my $subref="\\&$unique_name";
         while ($header =~ s/^([^\#]*?) (?<! :) :\s* (\w+) \s* (?: \(\s* ($balanced_re) \s*\)\s*)/$1/x) {
            push @opts, "$2=>'$3'";
         }
         if ($header =~ s/^([^\#]*?) (?<!:) (:\s* $rule_input_re)/$1/xo) {
            if (!$method || !$user || $global) {
               push @{$self->buffer}, "BEGIN { die 'only user_methods can have rule-like input properties' }\n";
               return;
            }
            if ($type_params) {
               push @{$self->buffer}, "BEGIN { die 'rule-like method can't have explicit type parameters' }\n";
            }
            $subref="self(1)->add_method_rule('$labels$2', $subref)";
            push @opts, "label=>[ 'Polymake::Core::ObjectType::MethodAsRule' ]";
            $self->after_rule=1;
         } elsif (defined $labels) {
            push @opts, "label=>[ application::self()->add_labels('$labels') ]";
         }

         if (defined $signature) {
            add_overloaded_instance($self, $header, $context_check, $user, $global,
                                    $name, $method, $unique_name, $subref, $signature, $type_params,
                                    $typecheck, join(", ", @opts))
              or return;
         } else {
            provide_short_prologue($self, $header, $display_credit) if ($user || $global) && $self->credit_seen;

            push @{$self->buffer},
                 prepare_context_check($self, $name, $tparams, $context_check) .
                 " add$global Overload('$name', $subref, undef, ".join(", ", @opts)."); sub ${unique_name}${meth_decl}$header\n";
         }

      } elsif ($global) {
         push @{$self->buffer},
              "BEGIN { die 'global method must have a signature and/or labels' }\n";
         return;

      } else {
         if ($plausibility_checks) {
            if ($name eq "construct") {
               push @{$self->buffer},
                   "BEGIN { die '\\'construct\\' must be defined as an overloaded method' }\n";
               return;

            } elsif ($type_method) {
               if (string_list_index(\@PropertyType::override_methods, $name) < 0) {
                  push @{$self->buffer},
                       "BEGIN { die 'unknown type_method $name' }\n";
                  return;
               }
            } else {
               $context_check="BEGIN { $context_check exists &$name and Core::multiple_func_definition() }";
            }
         } else {
            $context_check="";
         }
         if (defined $type_params) {
            push @{$self->buffer},
                 "BEGIN { die 'parameterized function must have a signature' }\n";

         } elsif ($header =~ s/^([^\#]*?) (?<!:) (:\s* $rule_input_re)/$1/xo) {
            unless ($method && $user) {
               push @{$self->buffer},
                    "BEGIN { die 'only user_methods can have rule-like input properties' }\n";
               return;
            }
            ++$funcnt;
            provide_short_prologue($self, $header, $display_credit) if $self->credit_seen;
            push @{$self->buffer},
                 "$context_check self(1)->add_method_rule('$2', \\&__meth__$funcnt, '$name'); sub __meth__$funcnt : method $header\n";
            $self->after_rule=1;

         } elsif ($header =~ /^\s* = \s* ($hier_id_re) \s*;\s*$/xo) {
            unless ($method && $user) {
               push @{$self->buffer},
                    "BEGIN { die 'a short-cut to a property must be declared as user_method' }\n";
               return;
            }
            push @{$self->buffer},
                 "$context_check sub $name$meth_decl { \$_[0]->give('$1') }\n";

         } else {
            provide_short_prologue($self, $header, $display_credit) if $user && $self->credit_seen;
            if ($type_method) {
               ++$funcnt;
               my $unique_name="__${name}__TP__$funcnt";
               push @{$self->buffer},
                    "self(1)->$name=Struct::pass_original_object(\\&$unique_name); sub $unique_name$meth_decl $header\n";
            } else {
               push @{$self->buffer},
                    "$context_check sub $name$meth_decl $header\n";
            }
         }
      }

   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid function header' }\n";
   }
}
#################################################################################
sub fill_help {
   my ($self, $whence, $path, $signature)=@_;
   return if $self->len_comments
          && $self->buffer->[$self->start_comments] =~ /^\# (?: \s* \@$id_re)* \s* \@hide (?: \s* \@$id_re)* \s*$/xio;

   my $topic= $whence ? "$whence->help_topic(1)" : "application::self()->help";
   push @{$self->buffer},
        "$topic->add([$path], <<'_#_#_#_', $signature);\n",
        cut_comments($self),
        "_#_#_#_\n",
        "#line ".$self->header_line."\n";
}

sub cut_comments {
   my ($self)=@_;
   if ($self->from_embedded_rules) {
      map { split /(?<=\n)/ } map { s/(?<!^)(?<![\#\n])(?=\#)/\n/mg; $_ }
      splice @{$self->buffer}, $self->start_comments, $self->len_comments;
   } else {
      splice @{$self->buffer}, $self->start_comments, $self->len_comments;
   }
}

sub scan_comments {
   my ($self)=@_;
   @{$self->buffer}[$self->start_comments .. $self->start_comments + $self->len_comments - 1];
}

sub append_comments {
   my $self=shift;
   splice @{$self->buffer}, $self->start_comments+$self->len_comments, 0, @_;
   $self->len_comments += @_;
}
#################################################################################
sub custom_hash_filter {
   my ($self, $watch_for_includes)=@_;
   my $last_comment_block=$#{$self->trailer};
   return sub : method {
      my ($self, $line)=@_;
      if ($line =~ /^\s* (?: (?'key' \w+) | (['"])(?'key' .*?)\2 ) \s* => [^\#\n]+ (?:\#\s* (?'type' $type_re))?/xo) {
         my ($key, $type)=@+{qw(key type)};
         if (defined $type) {
            # description in the trailing comments after the key => value pair
            splice @{$self->trailer}, -1, 0, "# \@key $type $key $'";

         } else {
            # assume description to be in the comment lines above
            $self->trailer->[$last_comment_block] =~ s/^\s*\#\s* ($type_re)/# \@key $1 $key/xo
              or croak("key comment block does not start with a valid type expression");
         }
         $last_comment_block=$#{$self->trailer};

      } elsif ($watch_for_includes && $line =~ /^\s* %($qual_id_re) \s*,?/xo) {
         splice @{$self->trailer}, -1, 0, "# \@relates $1\n";
         $last_comment_block=$#{$self->trailer};

      } elsif ($line =~ /^\s*\#\s*\S/) {
         splice @{$self->trailer}, -1, 0, $line;

      } elsif ($line =~ /^\s*\)\s*;\s*$/) {
         # end of the list
         undef $self->filter;
      }
   };
}

sub erase_comments_filter : method {
   my $self=shift;
   unless ($_[0] =~ s/\#.*$//) {
      # an empty line marks the end of the block
      undef $self->filter if $_[0] !~ /\S/;
   }
}
#################################################################################

sub fill {
   my $self=shift;
   $self->start_comments=$self->len_comments=0;
   my $trailer_pending=@{$self->trailer};

 READ: {
      my $line=readline $self->handle;
      if (!length($line)) {
         # EOF
         my $lastline=$.;
         close $self->handle or die "Syntax error near the end of file at ", $self->path, ", line ", $self->header_line, "\n";
         if (@{$self->trailer}) {
            my $firstline=$lastline-@{$self->buffer}+1;
            unshift @{$self->buffer},
                    $accurate_linenumbers && $trailer_pending > 1
                    ? ( "#line ".$self->injected_lines." \"" . $self->injected_source . "\"\n" ) : (),
                    @{$self->trailer},
                    $trailer_pending > 1
                    ? ( "#line $firstline" . ($accurate_linenumbers && " \"" . $self->path . "\"") . "\n" ) : ();
         }
         if ($self->preamble_end) {
            $self->application->preamble_end->{$self->rule_key}=$self->preamble_end;
         }
         push @{$self->buffer}, "1; __END__\n";
         if ($self->from_embedded_rules) {
            push @{$self->buffer_phase1}, "1; __END__\n";
         }
         $self->header_line=$lastline;
         last;
      }

      if ($line =~ /^(?:(\#line)\s+\d+(.*)|\s*)?$/) {
         # empty line
         my $set_line_number=defined($1);
         if (length($2) && $self->from_embedded_rules) {
            $self->credit_seen= $extension && defined($extension->credit);
         }
         if ($self->filter) {
            $self->filter->($line);
            if ($self->filter) {
               push @{$self->buffer}, $line;
               redo;
            }
         }

         if (@{$self->trailer}) {
            push @{$self->buffer},
                 $accurate_linenumbers && $trailer_pending > 1
                 ? ( "#line ".$self->injected_lines." \"" . $self->injected_source . "\"\n" ) : (),
                 splice(@{$self->trailer}, 0),
                 $set_line_number
                 ? ( $line ) :
                 $trailer_pending > 1
                 ? ( "#line $." . ($accurate_linenumbers && " \"" . $self->path . "\"") . "\n", $line ) : ();

            if ($accurate_linenumbers) {
               $self->injected_lines += $trailer_pending+1;
            }
            $self->gap=1;
            redo;
         }

         if ($self->gap==2) {
            my $digest_comment;
            if ($self->buffer->[$self->start_comments] =~ m{^\s*\#\s* \@topic \s+ (?: (?'cat' category) \s+ (?'path' (?'lead' \w+) (?:/.*(?<!\s))?+ )
                                                                                    | (?'path' (?'lead' \w+) (?:/\S+)?) (?:\s* (?'sig' \($balanced_re\)) )? ) \s*$}xi) {
               if ($+{path} eq "custom") {
                  $digest_comment="application::self()->custom->pkg_help->{__PACKAGE__}=<<'_#_#_#_';\n";
               } elsif ($Help::gather) {
                  my ($path, $lead, $cat, $sig)=@+{qw(path lead cat sig)};
                  my $signature= defined($sig) ? ", q$sig" : defined($cat) && ", '$cat'";
                  if ($path !~ m{/any/} and $lead eq "properties" || $lead eq "methods") {
                     $digest_comment="self(1)->help_topic(1)->add('$path', <<'_#_#_#_'$signature);\n";
                  } else {
                     $digest_comment="application::self()->help->add('" . ($path ne "application" && $path) . "', <<'_#_#_#_'$signature);\n";
                  }
               }
            }
            if (defined($digest_comment)) {
               $self->buffer->[$self->start_comments]=$digest_comment;
               if ($set_line_number) {
                  push @{$self->buffer}, "_#_#_#_\n";
               } else {
                  $line="_#_#_#_\n";
               }
               $self->start_comments=$self->len_comments=0;
            }
         }

         $self->gap=1;
         push @{$self->buffer}, $line;
         redo;
      }

      if ($line =~ /^\s*\#/) {
         # comment line
         if ($self->gap) {
            if ($self->gap==1) {
               # ... after an empty line - starts a new comment block
               $self->start_comments=@{$self->buffer}; $self->len_comments=0;
               $self->gap=2;
            }
            push @{$self->buffer}, $line;
            ++$self->len_comments;
            redo;
         }
         # comments amidst the code
         if ($self->filter) {
            $self->filter->($line);
         }
         push @{$self->buffer}, $line;
         last;
      }

      if (!$self->gap && !$self->after_rule || !$trailer_pending && compiling_in_sub()) {
         if ($self->filter) {
            $self->filter->($line);
         }
         push @{$self->buffer}, $line;
         last;
      }

      $self->gap=0;
      if ($line =~ m{^[ \t]* (?> (declare \s+)?) ($id_re) (?: $|\s+|(?= <))}xo  and
          my $header_sub= $1 ? $self->decl_rule_header_table->{$2} : $self->rule_header_table->{$2} || ($self->after_rule && $rule_subheaders{$2})) {

         # header recognized
         $line=$';
         $self->header_line=$.;
         $self->after_rule=0;
         $self->prolonged=0;

         # concatenate header continuation lines
         for (;;) {
            chomp $line;
            last if substr($line,-1,1) ne '\\';
            if (defined (my $cont=readline($self->handle))) {
               $cont =~ s/^\s{2,}/ /;
               substr($line,-1,1)=$cont;
               ++$self->prolonged;
            } else {
               push @{$self->buffer}, "die 'unexpected EOF after continuation mark';\n";
               return;
            }
         }
         $header_sub->($self, $line);
         if ($self->prolonged) {
            push @{$self->buffer}, ("\n") x $self->prolonged;
         }

      } else {
         # some perl code
         push @{$self->buffer}, $line;
      }
   }
}

#################################################################################
package Polymake::Core::Application;

sub configure {
   my ($self, $code, $rule_key, $line, $optional)=@_;
   if (($self->configured->{$rule_key} //= "0") > 0) {
      # successful configuration status from past sessions still valid
      1
   } else {
      require Polymake::Configure;
      my $success=eval { $code->() };
      if ($@) {
         warn_print( (caller)[1], ", line $line: autoconfiguration failed:\n", $@ ) if $Shell->interactive;
         $@="";
      }
      if (!$success && $optional) {
         $self->configured->{$rule_key}="0#";
         $success=1;
      }
      if ($success) {
         if ($line >= $self->preamble_end->{$rule_key}) {
            # no further CONFIGURE blocks follow
            substr($self->configured->{$rule_key},0,1)=$load_time;
            $self->custom->handler->need_save=1;
         }
         1
      } else {
         $self->configured->{$rule_key}=-$load_time;
         $self->custom->handler->need_save=1;
         $self->declared |= $has_failed_config;
         if (is_object(my $credit=$self->credits_by_rulefile->{$rule_key})) {
            $credit->shown=$Rule::Credit::hide;
         }
         $self->rulefiles->{$rule_key}=0;
      }
   }
}
#################################################################################
# private:
sub summarize_rule_prerequisites {
   my ($self, $failed, $rule_key, $line)=@_;
   if (defined($failed)) {
      $self->configured->{$rule_key}=$failed;
      $self->custom->handler->need_save=1;
      $self->declared |= $has_failed_config;
      if (is_object(my $credit=$self->credits_by_rulefile->{$rule_key})) {
         $credit->shown=$Rule::Credit::hide;
      }
      $self->rulefiles->{$rule_key}=0
   } else {
      if ($line >= $self->preamble_end->{$rule_key} && $self->configured->{$rule_key} =~ /^0\#?/) {
         # some CONFIGURE blocks passed and no further CONFIGURE blocks
         substr($self->configured->{$rule_key},0,1)=$load_time;
         $self->custom->handler->need_save=1;
      }
      1
   }
}
#################################################################################
sub include_required {
   my ($self, $block, $rule_key, $line)=@_;
   my @failed=include_rule_block($self, 1, $block);
   summarize_rule_prerequisites($self, @failed ? "0#rule:".join("|", @failed) : undef, $rule_key, $line);
}
#################################################################################
sub require_ext {
   my ($self, $block, $rule_key, $line)=@_;
   my (@failed, $success);
   $block =~ s/^\s+//;
   $block =~ s/\s+$//;
   foreach my $URI (split /\s+\|\s+/, $block) {
      if (defined (my $ext=$Extension::registered_by_URI{$URI})) {
         $success=$ext->is_active and last;
         push @failed, $URI;
      }
   }
   summarize_rule_prerequisites($self, $success ? undef : @failed ? "0#ext:".join("|", @failed) : "0", $rule_key, $line);
}
#################################################################################
sub add_credit {
   my ($self, $product, $ext_URI, $text, $rule_key, $may_repeat)=@_;
   if ($text =~ /\S/) {
      if (length($ext_URI)) {
         croak( "credit alias definition may not contain a credit note text" );
      }
      if ($plausibility_checks || $may_repeat) {
         if (my ($credit)=grep { defined } map { $_->credits->{$product} } $self, values %{$self->used}) {
            return $credit if $may_repeat;
            warn_print( "multiple credits for $product, using first description" );
            $self->credits_by_rulefile->{$rule_key}=$credit;
            return $credit;
         }
      }
      sanitize_help($text);
      my $credit=new Rule::Credit($product, $text);
      $self->credits_by_rulefile->{$rule_key}=$credit;
      $self->credits->{$product}=$credit;

   } elsif (length($ext_URI)) {
      my $ext=$Extension::registered_by_URI{$ext_URI}
        or croak( "unknown extension URI $ext_URI" );
      if ($plausibility_checks || $may_repeat) {
         if (my ($credit)=grep { defined } map { $_->credits->{$product} } $self, values %{$self->used}) {
            return $credit if $may_repeat;
            warn_print( "multiple credits for $product, using first description" );
         }
      }
      $self->credits->{$product}=$ext->credit
        or croak( "extension $ext_URI does not contain a credit note" );
   } else {
      &lookup_credit || croak( "CREDIT $product definition did not occur until here - either misspelled product name or missing REQUIRE block" );
   }
}
#################################################################################
my $loading_rule_key;

sub has_interactive_commands { $has_interactive_commands=shift; }

# private
sub start_preamble {
   my ($self, $rule_key, $code)=@_;
   $self->rule_code->{$rule_key}=$code;
   $self
}

# private
sub process_included_rule {
   my ($self, $rulefile, $filename, $ext, $rule_key, $rc)=@_;
   return $rc if defined($rc);
   if (defined (my $config_state=$self->configured->{$rule_key})) {
      if ($config_state > 0) {
         if ($config_state < ($ext // $self)->configured_at) {
            # enforce reconfiguration
            delete $self->configured->{$rule_key};
            $self->custom->handler->need_save=1;
         }
      } else {
         $self->rulefiles->{$rule_key}=0;
         if (is_string($config_state)
               and
             my ($on_rule, $prereqs)= $config_state =~ /^0\#(?:(rule)|ext):(.*)/) {
            # direct dependency on another rulefile or extension
            my ($revived, @depends_on, $prereq_rule, $prereq_app, $prereq_ext);
            if ($on_rule) {
               foreach $prereq_rule (split /\|/, $prereqs) {
                  $prereq_app= $prereq_rule =~ s/^($id_re):://o ? ($self->used->{$1} or return 0) : $self;
                  my $prereq_rule_key= $prereq_rule . ($ext && $ext->dir ne $prereq_app->installTop && '@'.$ext->URI);
                  if ($prereq_app->rulefiles->{$prereq_rule_key}) {
                     $revived=1;
                     last;
                  } elsif ($has_interactive_commands) {
                     push @depends_on, $prereq_app, $prereq_rule_key;
                  }
               }
            } else {
               foreach my $URI (split /\|/, $prereqs) {
                  if (defined (my $prereq_ext=$Extension::registered_by_URI{$URI})) {
                     if ($prereq_ext->is_active) {
                        $revived=1;
                        last;
                     } elsif ($has_interactive_commands && !$prereq_ext->is_bundled) {
                        push @depends_on, $prereq_ext;
                     }
                  }
               }
            }
            unless ($revived) {
               if ($has_interactive_commands) {
                  store_rule_to_wake($self, $rulefile, $filename, $ext, $rule_key, $on_rule, @depends_on);
               }
               return 0;
            }

         } elsif (-$config_state >= ($ext || $self)->configured_at) {
            # last configuration was failed, and nothing changed in between
            return 0;
         }
         # prerequisite was successfully reconfigured in the meanwhile, or reconfiguration forced
         delete $self->configured->{$rule_key};
         $self->custom->handler->need_save=1;
      }
   }
   local $extension=$ext if $ext != $extension;
   parse_rulefile($self, $rule_key, $filename);
}

# private
sub parse_rulefile {
   my ($self, $rule_key, $filename)=@_;
   $self->rulefiles->{$rule_key}=1;
   dbg_print( "reading rules from $filename" ) if $Verbose::rules>1;
   $loading_rule_key=$rule_key;
   require "rules:$filename";
   $self->rulefiles->{$rule_key};
}

# freeze the lexical enviroment (namespace lookup)
my $namespace_decls=<<'_#_#_#_';
self()->eval_expr=sub { eval $_[0] };
namespaces::memorize_lexical_scope;
namespaces::export_sub(undef, \&Polymake::temporary);
_#_#_#_

# named constant to be used in put(), take(), and add()
sub Polymake::temporary() { $PropertyValue::is_temporary }

# private:
# must be qualified, otherwise would land in main::
sub Polymake::Core::Application::INC {
   my ($self, $filename)=@_;
   my $handle;
   if (defined($self->compile_scope) && $filename =~ s/^(?:(rules)|c\+\+:(\d))://) {
      my ($prologue, $from_embedded_rules, $main_init);
      if ($1) {
         open $handle, $filename or die "can't read rule file $filename: $!\n";
         if (!$self->declared) {
            $prologue=$namespace_decls;
            $self->declared=$namespace_declared;
         }
         if (!($self->declared & $cpp_load_initiated)) {
            $self->declared |= $cpp_load_initiated;
            if ($self->declared & $main_init_closed) {
               $prologue=$cpp_init;
            } else {
               # $cpp_init will be injected after the IMPORT clauses
               $main_init=1;
            }
         }
      } else {
         $loading_rule_key=$filename;
         $from_embedded_rules=$2;
         if ($from_embedded_rules==1) {
            $handle=$self->cpp->embedded_rules_handle;
         }
      }
      my $app_pkg=$self->pkg;
      $self->compile_scope->begin_locals;
      local *application::=get_pkg($app_pkg);
      $self->compile_scope->end_locals;
      my $credit_val= $extension && '=$Polymake::Core::Application::extension->credit';
      if ($from_embedded_rules>=2) {
         if ($from_embedded_rules==3) {
            # renewing the preamble for a suspended fragment
            unshift @{$self->cpp->embedded_rules},
                    "$warn_options;\n",
                    "use namespaces '$app_pkg';\n",
                    "package $app_pkg;\n",
                    "my \$__cur_credit$credit_val;\n";
            push @{$self->cpp->embedded_rules},
                 "1; __END__\n";
         }
         return (\&CPlusPlus::perApplication::get_transformed_embedded, $self->cpp);
      }
      new RuleFilter($handle, $self, $filename, $loading_rule_key, $from_embedded_rules,
                     length($credit_val)>0 && defined($extension->credit), $main_init,
                     <<"_#_#_#_");
$warn_options;
use namespaces '$app_pkg';
package $app_pkg;
${prologue}my \$__cur_credit$credit_val;
my \$__preamble_end=self()->preamble_end->{'$loading_rule_key'};
#line 1 "$filename"
_#_#_#_

   } elsif ($filename =~ s/^script([:=])//) {
      open $handle, $filename or die "can't read script file $filename: $!\n";
      my $tm=(stat $handle)[9];
      new ScriptLoader($handle, [ "package Polymake::User; $warn_options; use " . ($1 eq ':' ? "application undef, " : "namespaces ") . "\$namespaces::destroy_declare;\n",
                                  "Polymake::Core::StoredScript->new(\"$filename\", $tm, Polymake::Core::rescue_static_code(1));\n",
                                  "#line 1 \"$filename\"\n"
                                ]);

   } elsif (defined (my $full_path=find_file($filename, @{$self->myINC}))) {
      open $handle, $full_path or die "can't read file $full_path: $!\n";
      new ScriptLoader($handle, [ "$warn_options; use namespaces \"".$self->pkg."\";\n",
                                  "#line 1 \"$full_path\"\n"
                                ]);
   } else {
      undef
   }
}
####################################################################################

$INC{"application.pm"}=$INC{"Polymake/Core/Application.pm"};

package application;

sub import {
   (undef, my ($app_name, $flags))=@_;
   if (defined $app_name) {
      package Polymake::User;
      eval {
         namespaces::temp_disable();
         application($app_name);
      };
      if ($@) {
         err_print($@);
         exit 1;
      }
      $INC[0]=$application if $INC[0] != \&Polymake::Core::Shell::interactive_INC;
      import namespaces(1, $application->eval_expr);
      namespaces::import_subs();
   } else {
      import namespaces(1 | $flags, $Polymake::User::application->eval_expr);
   }
}

$namespaces::special_imports{"application.pm"}=1;

####################################################################################
package Polymake::Core::NeutralScriptLoader;

# fake object mimicking an empty Application
use Polymake::Struct (
   [ '@ISA' => 'Application' ],
   [ new => '' ],
   [ '$name' => '"none"' ],
);

*new=\&_new;

sub Polymake::Core::NeutralScriptLoader::INC {
   my ($self, $filename)=@_;
   if ($filename =~ s/^script[:=]//) {
      open my $handle, $filename or die "can't read script file $filename: $!\n";
      new ScriptLoader($handle, [ "package Polymake::User; $warn_options; use namespaces;\n",
                                  "#line 1 \"$filename\"\n"
                                ]);
   } elsif ($User::application != $self) {
      $User::application->INC($filename);
   } else {
      undef;
   }
}

####################################################################################
my %script_repo;

package Polymake::Core::StoredScript;
use Polymake::Struct (
   [ 'new' => '$$$' ],
   [ '$path' => '#1' ],
   [ '$timestamp' => '#2' ],
   [ '&code' => '#3' ],
);

sub new {
   my $self=&_new;
   $script_repo{$self->path}=$self;
}

sub locate_file {
   my $filename=shift;
   my $full_path;
   if (defined ($full_path=find_file_in_path($filename, $User::application->scriptpath))) {
      return ($full_path, (stat _)[9], $User::application);
   }
   foreach my $app (values %{$User::application->imported}) {
      if (defined ($full_path=find_file_in_path($filename, $app->scriptpath))) {
         return ($full_path, (stat _)[9], $app);
      }
   }
   if (-f ($full_path="$InstallTop/scripts/$filename")) {
      return ($full_path, (stat _)[9]);
   }
   foreach my $ext (@Extension::active[$Extension::num_bundled..$#Extension::active]) {
      if (-f ($full_path=$ext->dir."/scripts/$filename")) {
         return ($full_path, (stat _)[9]);
      }
   }
   if (defined ($full_path=find_file_in_path($filename, \@User::lookup_scripts))) {
      return ($full_path, (stat _)[9]);
   }
   if (-f $filename) {
      my $tm=(stat _)[9];
      return (Cwd::abs_path($filename), $tm);
   }
   ();
}

sub find {
   my $filename=pop;
   if (my ($path, $tm, $app)=locate_file($filename)) {
      if (defined (my $script=$script_repo{$path})) {
         if ($tm==$script->timestamp) {
            return $script->code;
         }
      }
      (undef, $path, $app);
   } else {
      croak( "script file \"$filename\" not found" );
   }
}

#################################################################################
# A "virtual" package:
# per default offers dummy methods which do not store anything.
# Should be redefined when needed

package Polymake::Core::Help;

my $dummy=bless [ ];

sub clone { $dummy }
sub add {}
*add_function=\&add;
*merge=\&add;
sub get { undef }
sub related { [ ] }

my $impl;
declare $gather;
sub redefine { $gather=defined($impl=$_[1] || caller) }

sub new {
   if ($impl) {
      shift;
      $impl->new(@_);
   } else {
      $dummy;
   }
}

####################################################################################

# pre-declare the package as to enable it for namespace lookup in CONFIGURE clauses
package Polymake::Configure;

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
