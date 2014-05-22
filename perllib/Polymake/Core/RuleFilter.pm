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

use strict;
use namespaces;

#################################################################################
package Polymake::Core;

sub multiple_func_definition {
   croak( "Multiple definition of a non-overloaded function" );
}

sub multiple_type_definition {
   croak( "Multiple definition of a property type" );
}

sub check_application_pkg {
   my $app_expected=$_[0];
   if (compiling_in(\%application::) != $app_expected) {
      croak( "This declaration ",
             $app_expected ? "is only allowed" : "is not allowed",
             " in the top-level (application) scope" );
   }
}

declare $warn_options="use strict 'refs'; use warnings 'void', 'syntax'";

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

my (%main_init_rule_headers, %rule_headers, %main_init_decl_headers, %decl_headers);

use Polymake::Struct (
   [ new => '$$$$$$$@' ],
   [ '$handle' => '#1' ],
   [ '$application' => '#2' ],
   [ '$path' => '#3' ],
   [ '$rule_key' => '#4' ],
   [ '$from_embedded_rules' => '#5' ],
   '@buffer',
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
   if (@_>1) {
      @{$self->buffer}=@_;
   } else {
      @{$self->buffer}=split /(?<=\n)/, shift;
   }
   if ($accurate_linenumbers) {
      $self->injected_lines=@{$self->buffer}+1;
      $self->application =~ /0x[0-9a-f]+/;
      $self->injected_source="/loader/$&/rules:".$self->path;
   }
   (\&get, $self);
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

   object => sub { reopen_type(@_, "object") },

   property_type => sub { reopen_type(@_, "property") },

   default_object => sub {
      my ($self, $header)=@_;
      if ($header =~ /($type_re) \s*;$/xo) {
         push @{$self->buffer},
              "application::self()->default_type=application::self()->eval_type('$1');\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid type name' }\n";
      }
   },

   property => sub {
      my ($self, $header)=@_;
      if ($header =~ /^ (?'prop_name' $id_re) \s*:\s* (?'prop_type' $type_re) \s* (?'attrs' (?: : [^:;{=]++ )*)
                                                  (?: (?'default' =\s*[^;]++)? ; | (?'open_scope' \{)) \s*$/xo) {

         my ($prop_name, $prop_type, $attrs, $default, $open_scope)=@+{qw(prop_name prop_type attrs default open_scope)};

         translate_type($prop_type);
         fill_help($self, "self(1)", "'properties', '$prop_name'") if $Help::gather;
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
              "self(1)->add_property_definition('$prop_name', $prop_type, @attrs); " .
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
                 "self(1)->help_topic(1)->add(['properties', '$prop_name'], <<'_#_#_#_');\n",
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

   function => sub { prepare_function(@_, 0, "", 0) },

   method => sub { prepare_function(@_, 1, "", 0, "Core::check_application_pkg(0);") },

   user_function => sub { prepare_function(@_, 0, "", 1, !$_[0]->from_embedded_rules && "Core::check_application_pkg(1);") },

   user_method => sub { prepare_function(@_, 1, "", 1, "Core::check_application_pkg(0);") },

   global_method => sub { prepare_function(@_, 1, "_global", 0, "Core::check_application_pkg(0);") },

   typecheck => \&prepare_typecheck,

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
         my $type=$1;
         translate_type($type);
         $self->after_rule=1;
         push @{$self->buffer},
              "self(1)->add_auto_cast($type);\n";

      } else {
         push @{$self->buffer}, "BEGIN { die 'invalid auto_cast declaration' }\n";
      }
   },
);

sub provide_func_prologue {
   if ($_[1] =~ s/(?: ^ | \bsub \s+ $id_re) \s* \K \{/$_[2]/xo) {
      undef $_[0]->filter;
   }
}

sub provide_rule_prologue : method {
   provide_func_prologue(@_, '{ my $this=shift;');
}

sub inject_credit_display : method {
   provide_func_prologue(@_, '{ $__cur_credit->display if $__cur_credit && $Verbose::credits > $__cur_credit->shown;');
}

sub provide_credit_display {
   if ($_[0]->credit_seen) {
      $_[0]->filter=\&inject_credit_display;
      &inject_credit_display;
   }
}

%decl_headers=(
   object => \&process_object_decl,

   property_type => \&process_property_type,

   permutation => \&process_permutation_decl,
);

while (my ($keyword, $code)=each %main_init_rule_headers) {
   $rule_headers{$keyword} //= $code;
}
while (my ($keyword, $code)=each %rule_headers) {
   $main_init_rule_headers{$keyword} //= sub { &close_main_init_preamble; &$code; };
}
while (my ($keyword, $code)=each %decl_headers) {
   $main_init_decl_headers{$keyword} = sub { &close_main_init_preamble; &$code; };
}

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


my %rule_subheaders=(
   precondition => sub {
      my ($self, $header)=@_;
      if ($header =~ /^ : (?: \s* (?: !\s*)? (?:exists|defined) \s*\(\s* $hier_id_alt_re \s*\)\s* (?: , | ;\s*$ ) )+/xo) {
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
            translate_type($super);
         }
         $line.="application::self()->append_overriden_rule(self(1), $super, '$label'); ";
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
   "application::self()" . ($start && $has_interactive_commands && "->start_preamble('".$self->rule_key."', Polymake::Core::rescue_static_code(0))")
}
#####################################################################################################
#  helper routines common for object and property type declaration processing

sub process_template_params($$\@\@$\@\@) {
   my ($text, $super_text, $param_names, $prologue, $typecheck, $super_abstract, $super_instance)=@_;

   my ($i, $defaults_seen)=(0,0);
   while ($text =~ m{\G $type_param_re \s* (?: ,\s* | $ ) }xog) {
      my ($name, $default)=@+{qw(name default)};
      push @$param_names, $name;
      if (defined $default) {
         if ($default =~ /^\s*\{/) {
            translate_type_expr($default);
            $default="do $default";
         } else {
            translate_type($default);
         }
         for ($i=0; $i<$#$param_names; ++$i) {
            $default =~ s/\b(?<!:)$param_names->[$i](?:->type)/\$_[$i]/g;
         }
         push @$prologue,
              "    push \@_, $default if \$#_ < $i;\n";
         ++$defaults_seen;

      } elsif ($defaults_seen) {
         return "wrong order of parameters with and without default types";
      }
   }

   if (defined $typecheck) {
      for ($i=0; $i<=$#$param_names; ++$i) {
         $typecheck =~ s/\b$param_names->[$i]\b/\$_[$i]/g;
      }
      $_[4]="$typecheck;";
   }

   if (@$param_names>1) {
      push @$prologue,
           "    my \$type_inst=\$type_inst;\n",
           "    foreach my \$arg (\@_[0.." . ($#$param_names-1) ."]) { \$type_inst=( \$type_inst->{\$arg} ||= { } ) }\n",
           "    \$type_inst->{\$_[$#$param_names]}\n";
   } else {
      push @$prologue,
           "    \$type_inst->{\$_[0]}\n",
   }

   if ($defaults_seen) {
      unshift @$prologue,
         $#$param_names >= $defaults_seen ?
         ( "    croak('too few type parameters for ', __PACKAGE__) if \$#_ < " . ($#$param_names - $defaults_seen) . ";\n" ) : (),
           "    croak('too many type parameters for ', __PACKAGE__) if \$#_ > $#$param_names;\n";
   } else {
      unshift @$prologue,
           "    croak('wrong number of type parameters for ', __PACKAGE__) if \$#_ != $#$param_names;\n";
   }

   while ($super_text =~ m{\G (?'expr' (?> (?: $id_re \s*=>\s*)?) (?'type' $qual_id_re) (?: \s* (?= <)(?'tparams' $confined_re) )?) \s*(?:,\s*)?}xgo) {
      my ($super_expr, $super_type, $tparams)=@+{qw(expr type tparams)};
      if (defined $3) {
         my $dependent;
         for ($i=0; $i<=$#$param_names; ++$i) {
            $dependent += $super_expr =~ s/\b$param_names->[$i]\b/\$_[$i]/g;
         }
         translate_type($super_expr);
         if ($dependent) {
            push @$super_abstract, "$super_type->self";
            push @$super_instance, $super_expr;
         } else {
            push @$super_abstract, $super_expr;
         }
      } else {
         push @$super_abstract, "$super_expr->type";
      }
   }

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
   my $type_inst=my $root=$type_inst;
   $type_inst=( $type_inst->{$_} ||= { } ) for @_;
   $type_inst->{$root}
.

sub process_property_type {
   my ($self, $header)=@_;
   my $first_line=@{$self->buffer};

   if ($header =~ /^ ($id_re) \s*=\s* ($type_re) \s*;\s* $/xo) {
      my ($alias, $type)=@_;
      my $outer_pkg=compiling_in();
      translate_type($type);
      push @{$self->buffer},
           ($plausibility_checks
           ? "BEGIN { exists \$$outer_pkg\::{'$alias\::'} and Core::multiple_type_definition() } " : "") .
             "*$outer_pkg\::$alias\::=get_pkg($type->pkg);\n";

   } elsif ($header =~ /^ (?'type_name' $id_re) (?: $type_params_re | \s*<\s* (?'tparams' \.\.\. | \*) \s*> )?+
                                                (?: \s*:\s* (?!c\+\+)(?'super' $type_expr_re) )?+
                                                (?: \s*:\s* (?'cpp_binding' c\+\+) (?: \s*\( (?'cpp_opts' $balanced_re) \) )?+
                                                            (?: \s*:\s* (?'attr_name' $id_re) \s* \( (?'attr_value' $balanced_re) \) )? )?+
                          \s* (?: (?'open_scope' \{ ) | ; ) \s*$/xo) {

      my ($type_name, $tparams, $typecheck, $super, $cpp_binding, $cpp_opts, $attr_name, $attr_value, $open_scope)=@+{qw
         ( type_name   tparams  typecheck    super   cpp_binding   cpp_opts   attr_name   attr_value   open_scope)   };

      fill_help($self, "", "'property_types', '$type_name'") if $Help::gather;

      my $type_pkg=$self->application->pkg."::$type_name";
      push @{$self->buffer},
           "{ package $type_pkg; " . 
           ($plausibility_checks ? "BEGIN { exists &type and Core::multiple_type_definition(); } " : "") .
           "namespaces::memorize_lexical_scope;\n";

      my $buffer_size;
      if ($accurate_linenumbers && !$self->from_embedded_rules) {
         $buffer_size=@{$self->buffer};
         push @{$self->buffer},
              "#line ".$self->injected_lines." \"".$self->injected_source."\"\n";
      }

      my $appendix="";

      if (defined($tparams)) {
         # parameterized type template

         my (@param_names, @prologue, @super_abstract, @super_instance, $n_defaults);
         if ($tparams eq "...") {
            if (defined($super)) {
               $self->buffer->[$first_line]="BEGIN { die 'type with arbitrary many parameters may not be derived' }\n";
               return;
            }
            @prologue=@ellipsis_prologue;
            $n_defaults=0;
         } elsif ($tparams eq "*") {
            if (defined($super)) {
               if ($super =~ /</) {
                  push @super_abstract, "application::self()->eval_type('$super')->pkg";
               } else {
                  push @super_abstract, "namespaces::lookup_class(__PACKAGE__,'$super','application')";
               }
            }
            push @prologue, "    \$type_inst->{\$_[0]}\n";
            push @param_names, $tparams;
         } else {
            $n_defaults=process_template_params($tparams, $super, @param_names, @prologue, $typecheck, @super_abstract, @super_instance);
            if (is_string($n_defaults)) {
               $self->buffer->[$first_line]="BEGIN { die 'invalid property type declaration: $n_defaults' }\n";
               return;
            }
            $_.="->pkg" for @super_abstract;
         }

         push @{$self->buffer},
              "using namespaces 'Polymake::Core::PropertyParamedType';\n",
              "{ my \$type_inst={ };" . ($cpp_binding ? " my \$cpp_opts_sub;\n" : "\n");

         if ($cpp_binding) {
            if (@param_names) {
               $appendix='$app->cpp->add_template_instance($t,$cpp_opts_sub,$Polymake::Core::PropertyType::nesting_level);';
            } else {
               $self->buffer->[$first_line]="BEGIN { die 'type with arbitrary many parameters cannot have C++ binding' }\n";
               return;
            }
         }

         my $super_arg= @super_instance
                        ? "do { local_incr(\$Polymake::Core::PropertyType::nesting_level); @super_instance }"
                        : "undef";

         push @{$self->buffer},
              "  sub generic_type { shift unless is_object(\$_[0]);\n",
              @prologue,
              "    ||= do { my \$app=application::self();\n",
              defined($typecheck)
              ? "       $typecheck\n" : (),
              $tparams eq "*"
              ? "       my \$t=new Polymake::Core::PropertyTypeInstance('$type_name', __PACKAGE__, \$app, $super_arg, \@_);\n"
              : "       my \$t=new Polymake::Core::PropertyParamedType('$type_name', __PACKAGE__, \$app, $super_arg, \\\@_);\n",
              "       $appendix \$t } }\n",
              $tparams ne "*"
              ? "  new_generic Polymake::Core::PropertyParamedType('$type_name', __PACKAGE__, application::self(), [$n_defaults, qw(@param_names)]);\n" : (),
              @super_abstract
              ? "  \@ISA=(@super_abstract);  using namespaces \@ISA;\n" : (),
              @super_instance
              ? "  *rebind_type=Polymake::Core::PropertyParamedType::prepare_rebind(__PACKAGE__, \\&generic_type, q{$super}, qw(@param_names));\n"
              : "  sub rebind_type { undef }\n",
              $cpp_binding
              ? '  $cpp_opts_sub=application::self()->cpp->add_type_template(__PACKAGE__, template_params=>' . ($tparams eq "*" ? "'*'" : scalar(@param_names)) . ",$cpp_opts);\n"
              : (),
              "  *type=\\&generic_type;\n",
              (!$open_scope && "} ")."}\n";

      } else {
           # non-parameterized type
           if ($cpp_binding) {
              $cpp_opts =~ s/enum\s*(?=[({])($confined_re)/Polymake::Core::Application::RuleFilter::process_enum(q$1)/o;
              $cpp_opts =~ s/\bembedded\b/descr=>'embedded'/;
              if ($attr_name eq "subst_const_op") {
                 $attr_value =~ s/^\s*(\S+)\s*$/$1/;
              } elsif (defined $attr_name) {
                 $self->buffer->[$first_line]="BEGIN { die 'unknown type attribute \'$attr_name\'' }\n";
                 return;
              }
           }
           translate_type($super) if defined($super);

           push @{$self->buffer},
                "  my \$_type_inst=new Polymake::Core::PropertyType('$type_name', __PACKAGE__, application::self(), $super);\n",
                defined($super)
                ? "  \@ISA=( \$_type_inst->super->pkg ); using namespaces \@ISA;\n"
                : "  using namespaces 'Polymake::Core::PropertyType';\n",
                $cpp_binding
                ? "  application::self()->cpp->add_type(\$_type_inst,$cpp_opts);\n"
                : (),
                $attr_name
                ? "  namespaces::subst_const_op(application::self()->pkg, q<$attr_value>, sub { \$_type_inst->parse_string->(\@_) });\n"
                : (),
                $open_scope
                ? "  my \$_analyzer=bless \\\$_type_inst, 'Polymake::Core::PropertyType::Analyzer';\n"
                : "}\n";
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
sub reopen_type {
   my ($self, $header, $what)=@_;
   if (my ($type)= $header =~ /^$type_re \s*\{\s*$/xo) {
      my $pkg=$type;
      my $check_existence;
      if ($type =~ /</) {
         my $main=$`;
         $pkg=substr($type,$-[0]);
         translate_type($check_existence=$type);
         # can't call $proto->mangled_name here, since the XxxType object might not exist yet (declared above in the same rulefile);
         # but package requires a string literal, so we need to reconstruct the mangled name
         $pkg =~ s/\s+//g;
         while ($pkg =~ s/<(\w+)>/__$1/g) { }
         $pkg =~ s/,/_I_/g;
         while ($pkg =~ s/<(\w+)>/_A_${1}_Z/g) { }
         if ($main =~ /::/) {
            $pkg=namespaces::lookup_class($self->application->pkg, $main)."::$pkg";
         } else {
            $pkg=$self->application->pkg . "::$main$pkg";
         }
      } else {
         if ($pkg =~ /::/) {
            $pkg=namespaces::lookup_class($self->application->pkg, $pkg);
         } else {
            $pkg=$self->application->pkg . "::$pkg";
         }
         $check_existence="exists &type";
      }
      push @{$self->buffer},
           "{ package $pkg;  $check_existence or die 'unknown $what type'; local *object_type::=get_pkg(__PACKAGE__);\n";
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid $what type reference' }\n";
   }
}
#####################################################################################################
sub process_object_decl {
   my ($self, $header)=@_;
   my $first_line=@{$self->buffer};
   if ($header =~ /^ $paramed_decl_re (?: (?: \s*:\s* (?'super' $types_re))?+ \s*(?: ; | (?'open_scope' \{))
                                        | (?('tparams') | \s*=\s* (?'alias' $type_re) (?: \s*;)?)) \s*$/xo) {

      my ($type_name, $tparams, $typecheck, $super, $open_scope, $alias)=@+{qw(lead_name tparams typecheck super open_scope alias)};

      my $object_pkg=$self->application->pkg."::$type_name";
      if ($plausibility_checks) {
         push @{$self->buffer},
              "BEGIN { application::self()->multiple_object_definition('$type_name', q{$tparams}) }\n";
      }

      if (defined($alias)) {
         translate_type($alias);
         substr($self->buffer->[-1], -1, 0)=" *$object_pkg\::=get_pkg($alias->pkg);";

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
              "{ package $object_pkg; local *object_type::=get_pkg(__PACKAGE__); namespaces::memorize_lexical_scope;\n";

         if (defined($tparams)) {
            # parameterized template

            my (@param_names, @prologue, @super_abstract, @super_instance, $all_params_have_defaults);
            my $n_defaults=process_template_params($tparams, $super, @param_names, @prologue, $typecheck, @super_abstract, @super_instance);
            if (is_string($n_defaults)) {
               $self->buffer->[$first_line]="BEGIN { die 'invalid object declaration: $n_defaults' }\n";
               last;
            }

            push @{$self->buffer},
                 "{ my \$type_inst={ };\n",
                 "  sub generic_type { shift unless is_object(\$_[0]);\n",
                 @prologue,
                 "    ||= " . (defined($typecheck) && "do { $typecheck ")
                            . "new Polymake::Core::ObjectType('$type_name', undef, \\\@_, self(1), " . join(",", @super_instance) . ");"
                            . (defined($typecheck) && " } ") . "}\n",
                 "  new Polymake::Core::ObjectType('$type_name', application::self(), [$n_defaults, qw(@param_names)], " . join(",", @super_abstract) . ");\n",
                 "  *type=\\&generic_type\n",
                 "}\n";

            if ($n_defaults == @param_names) {
               push @{$self->buffer},
                    "application::self()->default_type ||= generic_type();\n";
            }
         } else {
            # non-parameterized type
            translate_type($super) if defined $super;

            push @{$self->buffer},
                 "new Polymake::Core::ObjectType('$type_name', application::self(), undef, $super);\n",
                 "application::self()->default_type ||= type();\n";
         }

         push @{$self->buffer}, "}\n" unless $open_scope;

         if ($accurate_linenumbers) {
            $self->injected_lines += @{$self->buffer}-$buffer_size;
            push @{$self->buffer}, "#line ".($.+1)." \"".$self->path."\"\n";
         } else {
            push @{$self->buffer}, "#line ".($.+1)."\n";
         }
         $self->prolonged=0;
      }
   } elsif ($header =~ /^(?'app_name' $id_re):: $paramed_decl_re \s*;$/xo) {
      # forward declaration

      my ($app_name, $type_name, $tparams)=@+{qw(app_name lead_name tparams)};

      if (defined (my $app=lookup Application($app_name))) {
         # application already loaded
         if ($plausibility_checks) {
            my $proto;
            if (defined ($proto=UNIVERSAL::can($app->pkg."::$type_name", "self")) &&
                instanceof ObjectType(($proto=$proto->(1)))) {
               if (defined (my $diag=$proto->check_consistency($tparams))) {
                  push @{$self->buffer},
                       "BEGIN { die '$diag' }\n";
               }
            } else {
               push @{$self->buffer},
                    "BEGIN { die 'forward declaration does not match any object type in application $app_name' }\n";
            }
         }

      } else {
         $app=forward Application($app_name);

         if ($accurate_linenumbers) {
            push @{$self->buffer},
                 "#line ".$self->injected_lines." \"".$self->injected_source."\"\n";
         }

         push @{$self->buffer},
              "{\n",
              $app->forward_decls,
              "  if (defined (my \$proto=UNIVERSAL::can('".$app->pkg."::$type_name', 'self'))) {\n",
              $plausibility_checks
              ? ( "     (\$proto)=\$proto->(1); my \$diag=\$proto->check_consistency(q{$tparams});\n",
                  "#line $.".($accurate_linenumbers && " \"".$self->path."\"")."\n",
                  "     \$diag && die \$diag;\n",
                  ($accurate_linenumbers ? "#line ".($self->injected_lines+10)." \"".$self->injected_source."\"\n" : ())
                ) : (),
              "  } else {\n";
         my $cnt=0;
         if (defined $tparams) {
            ++$cnt while $tparams =~ /$type_param_re/go;
         }
         push @{$self->buffer},
              "    new Polymake::Core::ObjectType::ForwardDecl('$type_name', self(), $cnt, $.);\n",
              "} }\n";

         if ($accurate_linenumbers) {
            $self->injected_lines += 20;
            push @{$self->buffer}, "#line ".($.+1)." \"".$self->path."\"\n";
         } else {
            push @{$self->buffer}, "#line ".($.+1)."\n";
         }
      }
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid object declaration' }\n";
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
sub prepare_function {
   my ($self, $header, $method, $global, $user, $context_check)=@_;
   if ($header =~ $labeled_sub_re) {
      # TODO: process typechecks
      my ($labels, $name, $template_params, $signature)=@+{qw(labels lead_name tparams signature)};
      $header=$';
      my ($kw, $sig);
      if (defined $signature) {
         if (($sig=$signature) =~ s/(^|[,;]\s*|\s+) ((?: \\?% | \{ ) .*)/$1%/x) {
            ($kw=", keywords=>[$2]") =~ s/(?<! \\) \s* %(?=$qual_id_re)/\\%/gxo         # pass hash references, not copies
            or $kw =~ s/\[\s*%\s*\]/[]/;
         }
         $sig="q{$sig}";
      } else {
         $sig="undef";
      }

      if ($user) {
         fill_help($self,
                   $method ? ("self(-1)", "'methods', '$name'")
                           : ("", "'functions', '$name'"),
                   "q($signature)") if $Help::gather;
         $self->application->EXPORT->{$name} ||= $method ? "meth" : "user";
      }

      my $meth_decl= $method ? " : method" : "";
      if ($header =~ s/^([^\#]*?) :\s* c\+\+ \s* (?: \(\s* ($balanced_re) \s*\)\s* )? (?=[:;\{])/$1/xo) {
         my $options=$2;
         if ($plausibility_checks) {
            if (!defined $signature) {
               $context_check.=" exists &$name and Core::multiple_func_definition();";
            }
         } else {
            $context_check="";
         }
         my @attrs=("method=>$method");
         while ($header =~ s/^([^\#]*?) :\s* (\w+) \s* (?: \(\s* ($balanced_re) \s*\)\s*)? (?=[:;\{])/$1/x) {
            push @attrs, "$2=>".( defined($3) ? "'$3'" : 1);
         }
         if ($header =~ /^\s* (?: ;\s* (?: \#.*)? $ | (\{))/x) {
            my $credit='undef';
            my $hybrid=defined($1);
            if ($hybrid) {
               ++$funcnt;
               my $subname="__${name}__OV__$funcnt";
               push @attrs, "ext_code=>\\&$subname";
               $header="; sub $subname $header";
               if ($user || $global) {
                  provide_credit_display($self, $header);
               }
            } elsif ($user || $global) {
               $credit='$__cur_credit';
            }
            if ($template_params) {
               $template_params="extra_template_params=>q{$template_params}, ";
               $context_check="sub $name; BEGIN { export_sub namespaces(\\&$name); $context_check }";
            } else {
               $context_check &&= "BEGIN { $context_check }";
            }
            $labels &&= ", label=>[ application::self()->add_labels('$labels') ]";
            push @{$self->buffer},
                 "$context_check add$global Overload(application::self()->cpp->add_auto_function('$name', $sig, $credit, {".join(",",@attrs)."$kw}, {$template_params$options})$labels)$header\n";
         } else {
            push @{$self->buffer}, "BEGIN { die 'unknown attributes for C++ " . ($method ? "method" : "function") . "' }\n";
         }

      } elsif (defined($signature) || defined($labels)) {
         $plausibility_checks or $context_check="";
         my @attrs=("context=>application::self()");
         ++$funcnt;
         my $subref="\\&__${name}__OV__$funcnt";
         while ($header =~ s/^([^\#]*?) (?<! :) :\s* (\w+) \s* (?: \(\s* ($balanced_re) \s*\)\s*)/$1/x) {
            push @attrs, "$2=>'$3'";
         }
         if ($header =~ s/^([^\#]*?) (?<!:) (:\s* $rule_input_re)/$1/xo) {
            if (!$method || !$user || $global) {
               push @{$self->buffer}, "BEGIN { die 'only user_methods can have rule-like input properties' }\n";
               return;
            }
            $subref="self(1)->add_method_rule('$labels$2', $subref$kw)";
            $kw &&= ", keywords=>\\\@Polymake::Overload::deferred_keywords";
            $labels=", label=>[ 'Polymake::Core::ObjectType::MethodAsRule' ]";
            $self->after_rule=1;
         } else {
            $labels &&= ", label=>[ application::self()->add_labels('$labels') ]";
         }
         if ($user || $global) {
            provide_credit_display($self, $header);
         }
         if ($template_params) {
            push @attrs, "extra_template_params=>q{$template_params}";
            $context_check="sub $name; BEGIN { export_sub namespaces(\\&$name); $context_check }";
         } else {
            $context_check &&= "BEGIN { $context_check }";
         }

         push @{$self->buffer},
              "$context_check add$global Overload('$name', $sig, $subref,".join(",",@attrs)."$kw$labels); sub __${name}__OV__$funcnt$meth_decl $header\n";

      } elsif ($global) {
         push @{$self->buffer},
              "BEGIN { die 'global method must have a signature and/or labels' }\n";
         return;

      } else {
         $context_check= $plausibility_checks ? "BEGIN { $context_check exists &$name and Core::multiple_func_definition() }" : "";
         if (defined $template_params) {
            push @{$self->buffer},
                 "BEGIN { die 'function template must have a signature' }\n";

         } elsif ($header =~ s/^([^\#]*?) (?<!:) (:\s* $rule_input_re)/$1/xo) {
            unless ($method && $user) {
               push @{$self->buffer},
                    "BEGIN { die 'only user_methods can have rule-like input properties' }\n";
               return;
            }
            provide_credit_display($self, $header);
            ++$funcnt;
            push @{$self->buffer},
                 "$context_check self(1)->add_method_rule('$2', \\&__meth__$funcnt, '$name'); sub __meth__$funcnt : method $header\n";
            $self->after_rule=1;

         } elsif ($header =~ /^\s* = \s* ($hier_id_re) \s*;\s*$/xo) {
            unless ($method && $user) {
               push @{$self->buffer},
                    "BEGIN { die 'only user_methods may be defined as short-cuts to other properties' }\n";
               return;
            }
            push @{$self->buffer},
                 "$context_check sub $name$meth_decl { \$_[0]->give('$1') }\n";

         } else {
            provide_credit_display($self, $header) if $user;
            push @{$self->buffer},
                 "$context_check sub $name$meth_decl $header\n";
         }
      }

   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid function header' }\n";
   }
}
#################################################################################
sub prepare_typecheck {
   my ($self, $header)=@_;
   if ($header =~ $sub_re) {
      my ($name, $signature)=@+{qw(name signature)};
      if (defined $signature) {
         $header=$';
         my $context_check= $plausibility_checks && "BEGIN { Core::check_application_pkg(1) }";
         ++$funcnt;
         my $subref="\\&__${name}__OV__$funcnt";
         get_pkg("application::typechecks", 1);
         push @{$self->buffer},
              "$context_check add Overload(__PACKAGE__.'::typechecks::$name', q{$signature}, $subref, typeofs=>1, context=>application::self()); sub __${name}__OV__$funcnt $header\n";
      } else {
         push @{$self->buffer}, "BEGIN { die 'typecheck function must have a signature' }\n";
      }
   } else {
      push @{$self->buffer}, "BEGIN { die 'invalid typecheck function header' }\n";
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
   my $self=shift;
   if ($self->from_embedded_rules) {
      map { split /(?<=\n)/ } map { s/(?<!^)(?<![\#\n])(?=\#)/\n/mg; $_ }
      splice @{$self->buffer}, $self->start_comments, $self->len_comments;
   } else {
      splice @{$self->buffer}, $self->start_comments, $self->len_comments;
   }
}

sub scan_comments {
   my $self=shift;
   @{$self->buffer}[$self->start_comments .. $self->start_comments + $self->len_comments - 1];
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
         $self->header_line=$lastline;
         last;
      }

      if ($line =~ /^(?:(\#line)\s+\d+.*|\s*)?$/) {
         # empty line
         my $set_line_number=defined($1);

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
                                                                                    | (?'global' globalcategory) \s+ (?'global_name' .+ ) \s*
                                                                                    | (?'path' (?'lead' \w+) (?:/\S+)?) (?:\s* (?'sig' \($balanced_re\)) )? ) \s*$}xi) {
               if ($+{path} eq "custom") {
                  $digest_comment="application::self()->custom->pkg_help->{__PACKAGE__}=<<'_#_#_#_';\n";
               } elsif ($Help::gather) {
                  my $signature= defined($+{sig}) ? ", q$+{sig}" : defined($+{cat}) && ", '$+{cat}'";
                  if ($+{lead} eq "properties" || $+{lead} eq "methods") {
                     $digest_comment="self(1)->help_topic(1)->add('$+{path}', <<'_#_#_#_'$signature);\n";
                  } elsif (defined $+{global}) {
                     $digest_comment="application::self()->help->add('global_categories/$+{global_name}', <<'_#_#_#_', 'category');\n";
                  } else {
                     $digest_comment="application::self()->help->add('" . ($+{path} ne "application" && $+{path}) . "', <<'_#_#_#_'$signature);\n";
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
      if ($line =~ m{^[ \t]* (?> (declare \s+)?) ($id_re) (?: $|\s+)}xo  and
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
sub multiple_object_definition {
   my ($self, $type_name, $tparams)=@_;
   no strict 'refs';
   if (exists ${ $self->pkg."::" }{"$type_name\::"}) {
      if (exists ${ $self->pkg."::$type_name\::" }{object_type}) {
         croak( "Multiple definition of an object type" );
      }
      if (defined (my $fwd_self=UNIVERSAL::can($self->pkg."::$type_name", "self"))) {
         ($fwd_self)=$fwd_self->(1);
         if (defined (my $diag=$fwd_self->check_consistency($tparams))) {
            croak( $diag );
         }
      }
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
   return $rc if defined $rc;
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
export_sub namespaces(\&Polymake::temporary);
_#_#_#_

# named constant to be used in put(), take(), and add()
sub Polymake::temporary() { $PropertyValue::is_temporary }

# private:
# must be qualified, otherwise would land in main::
sub Polymake::Core::Application::INC {
   my ($self, $filename)=@_;
   my $handle;
   if (defined($self->compile_scope) && $filename =~ s/^(?:(rules)|c\+\+)://) {
      my ($prologue, $from_embedded_rules, $main_init);
      if ($1) {
         open $handle, $filename or die "can't read rule file $filename: $!\n";
         if (!$self->declared) {
            $prologue=$namespace_decls;
            $self->declared = $namespace_declared;
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
         $from_embedded_rules=1;
         $handle=$self->cpp->embedded_rules_handle;
         $prologue=<<'_#_#_#_';
return Polymake::Core::rescue_static_code(1);
_#_#_#_
      }
      my $app_pkg=$self->pkg;
      $self->compile_scope->begin_locals;
      local *application::=get_pkg($app_pkg);
      $self->compile_scope->end_locals;
      my $credit_val= $extension && '=$Polymake::Core::Application::extension->credit';
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

# private:
sub forward_decls {
   my $self=shift;
   my $decls;
   if (!($self->declared & $namespace_declared)) {
      $decls=$namespace_decls;
      $self->declared |= $namespace_declared;
   }
   my $app_pkg=$self->pkg;
   split /(?<=\n)/, <<"_#_#_#_";
use namespaces '$app_pkg';
package $app_pkg;
$decls
_#_#_#_
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
