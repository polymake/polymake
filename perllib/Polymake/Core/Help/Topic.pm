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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

#################################################################################
#
#  Help class designed for interactive shell
#
package Polymake::Core::Help::Topic;

use Polymake::Struct (
   [ new => ';$$$' ],
   [ '$parent' => 'weak(#1)' ],
   [ '$name' => '#2' ],
   '$category',
   [ '$text' => '#3' ],
   [ '$defined_at' => 'undef' ],
   '@toc',
   '%topics',
   '@related',
   '%annex',
);

# between several entries with the same topic path
declare $separator = "-----\n";

# Global flag whether the point of definition of every help topic should be stored.
# This is relatively expensive and used only in the scripts generating documentation pages.
declare $store_provenience = false;

sub full_path {
   my ($self) = @_;
   my $path = "";
   do {
      $path = "/" . $self->name . $path;
   } while (defined($self = $self->parent));
   $path
}

# for the sake of interchangeability with BigObjectType and PropertyType
sub help_topic { shift }

sub top {
   my ($self)=@_;
   while (defined ($self->parent)) {
      $self = $self->parent;
   }
   $self
}

sub spez_topic { undef }

#################################################################################
my $stripped = qr{ [ \t]* (.*(?<!\s)) [ \t]*\n }xm;
my $help_line_start = qr{^[ \t]*\#[ \t]*}m;
my $hint_re = qr{\[ ($balanced_re) \]}x;
my $hints_re = qr{(?: $hint_re \s*)*}xo;

sub add {
   my ($self, $path, $text, $signature) = @_;
   unless (is_array($path)) {
      $path = [ split m'/', $path ];
   }
   my ($source_file, $source_line, @example_start_lines);
   if ($store_provenience) {
      ($source_file, $source_line)=(caller)[1,2];
      if ($source_file =~ m{(?:BigObjectType|Help/Topic)\.pm}) {
         ($source_file, $source_line) = (caller(1))[1, 2];
      }

      if ($text =~ /\@example\s/) {
         my @lines = split /\n/, $text;
         # in embedded rules, compiler sees everything on a single (last) line because of a macro
         my $line = $source_file =~ /\.(?:cc|cpp|C|h|hh|H)$/ ? $source_line-@lines : $source_line;
         foreach (@lines) {
            if (/$help_line_start \@example (?:\s|$)/xo) {
               push @example_start_lines, $line;
            }
            ++$line;
         }
      }
   }

   my ($cat, @related, %annex, $top);
   if (defined $text) {
      if ($text =~ /^\# \s* copyright \s+/xsi) {
         # can happen if some undocumented declaration immediately follows the rule file header
         $text = "UNDOCUMENTED\n";
      } else {
         $text =~ s/$help_line_start \@notest \s*$//xomi;

         if ($text =~ s/$help_line_start \@category $stripped//xomi) {
            splice @$path, -1, 0, $1;
            $cat = true;
         }

         if ($text =~ s/$help_line_start \@relates \s+ $stripped//xomi) {
            my $related = $1;
            $top = $self->top;
            @related = map {
               if (my @to_topic = index($_, "/") >= 0 ? $top->get_topics($_) : $top->find($_)) {
                  if (@to_topic > 1) {
                     croak( "help tag \@related '$_' refers to more than one topic, please qualify appropriately" );
                  }
                  ($to_topic[0], @{$to_topic[0]->related})
               } else {
                  croak( "help tag \@related '$_' refers to an unknown topic" );
               }
            } split /(?: \s*,\s* | \s+)/x, $related;
         }

         my ($private_options, @option_lists);
         while ($text =~ s/$help_line_start \@($id_re) (?:[ \t]+|$) ( .*\n (?:(?>$help_line_start) (?! \@$id_re) .*\n)* )//xom) {
            my ($tag, $value) = (lc($1), $2);
            sanitize_help($value);

            if ($tag eq "author" or $tag eq "display" or $tag eq "header") {
               $annex{$tag} = $value;

            } elsif ($tag eq "super") {
               if ($value =~ /^\s* ($type_re) \s*$/xo) {
                  $annex{$tag} = $1;
               } else {
                  croak( "help tag \@super '$value' does not refer to a valid type" );
               }
            } elsif ($tag eq "field") {
               if ($value =~ s/^\s* (?'type' $type_re) \s+ (?'name' $id_re) \s+//xo) {
                  push @{$annex{fields}}, new Param($+{name}, $+{type}, $value);
               } else {
                  croak( "help tag \@field '$value' does not start with a valid type and name" );
               }

            } elsif ($tag eq "return") {
               if ($value =~ s/^\s* ($type_re) \s*//xos) {
                  $annex{$tag} = new ReturnValue($1, $value);
               } else {
                  croak( "help tag \@return '$value' does not start with a valid type" );
               }
               $annex{function} = 0;

            } elsif ($tag eq "tparam") {
               if ($value =~ s/^\s* ($id_re) \s*//xos) {
                  push @{$annex{$tag}}, new Param($1, undef, $value);
               } else {
                  croak( "help tag \@tparam '$value' does not start with a valid name" );
               }

            } elsif ($tag eq "options") {
               if ($value =~ /^\s* %($qual_id_re) \s*$/xo) {
                  push @option_lists, $1;
               } else {
                  push @{$annex{options}}, $private_options = new Help(undef, 'options', $value);
                  $private_options->annex->{keys} = [ ];
               }
               $annex{function} = 0;

            } elsif ($tag eq "param") {
               if ($value =~ s/^\s* (?'hints' $hints_re) (?'type' $type_re) \s+ (?'name' $id_re (?: \s+\.\.\.)?) \s*//xos) {
                  if ($+{name} eq "the") {
                     croak( "\"the\" used as a parameter name" );
                  }
                  push @{$annex{$tag}}, new Param($+{name}, $+{type}, $value, $+{hints});
               } else {
                  croak( "help tag \@param '$value' does not start with valid type and name" );
               }
               $annex{function} = 0;

            } elsif ($tag eq "value") {
               if ($value =~ s/^\s* (?'name' $id_re) \s+ (?'value' $anon_quoted_re | $id_re ) \s+//xos) {
                  if (defined(my $param_list = $annex{param})) {
                     my ($param) = grep { $_->name eq $+{name} } @$param_list
                       or croak( "unknown parameter name '$+{name}'" );
                     $param->completer //= find Shell::Completion::Param('value_list');
                     push @{$param->values //= [ ]}, new ParamValue($+{value}, $value);
                  } else {
                     croak( "help tag \@value must follow parameter descriptions \@param" );
                  }
               } else {
                  croak( "help tag \@value '$value' does not start with valid name and quoted string or named constant" );
               }

            } elsif ($tag eq "option") {
               if ($value =~ s/^\s* (?'hints' $hints_re) (?'type' $type_re) \s+ (?'name' (?!\d)[\w-]+) \s*//xos) {
                  if (!defined($private_options)) {
                     push @{$annex{options}}, $private_options = new Help(undef, 'options', '');
                  }
                  push @{$private_options->annex->{keys} //= [ ]}, new Param($+{name}, $+{type}, $value, $+{hints});
               } else {
                  croak( "help tag \@option '$value' does not start with valid type and name" );
               }
               $annex{function} = 0;

            } elsif ($tag eq "key") {
               if ($value =~ s/^\s* (?'name' (?!\d)[\w-]+) \s+ (?'hints' $hints_re) (?'type' $type_re) \s*//xos) {
                  push @{$annex{keys}}, new Param($+{name}, $+{type}, $value, $+{hints});
               } else {
                  croak( "custom hash/option key doc string '$value' does not start with valid type" );
               }

            } elsif ($tag eq "option" || $tag eq "key") {
               if ($value =~ s/^\s* (?'hints' $hints_re) (?'type' $type_re) \s+ (?'name' (?!\d)[\w-]+) \s*//xos) {
                  if ($tag eq "key") {
                     push @{$annex{keys}}, new Param($+{name}, $+{type}, $value, $+{hints});
                  } else {
                     if (!defined($private_options)) {
                        push @{$annex{options}}, $private_options = new Help(undef, 'options', '');
                     }
                     push @{$private_options->annex->{keys} //= [ ]}, new Param($+{name}, $+{type}, $value, $+{hints});
                  }
               } else {
                  croak( "help tag \@$tag '$value' does not start with valid type and name" );
               }
               $annex{function} = 0 if $tag eq "option";

            } elsif ($tag eq "depends") {
               chomp $value;
               $annex{$tag} = $value;

            } elsif ($tag eq "example") {
               my @hints;
               while ($value =~ s/^\s*$hint_re\s*//so) {
                  push @hints, $1;
               }
               push @{$annex{examples}}, new Example($value, $source_file, shift(@example_start_lines), @hints);

            } else {
               croak( "unknown help tag \@$tag" );
            }
         }
         if ($signature =~ /[;{\@%]/) {
            my $mandatory = $`;
            my $cnt = 0;
            while ($mandatory =~ m/\G\s* (?: [*\$] (?: \s* (?:,\s*)? | $ )
                                           | $type_re (?: \s*\+ )? (?: \s+ | \s*,\s* | $ ))/xog) { ++$cnt; }
            $annex{mandatory} = $cnt-1;
            $annex{ellipsis} = true if $signature =~ /\@/;
            unless (@option_lists) {
               @option_lists = ($signature =~ /%($qual_id_re)/go);
            }
         }

         foreach (@option_lists) {
            $top //= $self->top;
            if (defined(my $opt_topic = $top->find("options", $_))) {
               if (is_array($opt_topic->annex->{keys})) {
                  push @{$annex{options}}, $opt_topic;
               }
            }
         }

         sanitize_help($text);
         if (!$cat && $text !~ /\S/) {
            $text = "UNDOCUMENTED\n";
         }
      }
   }

   if (@$path) {
      my $topic = pop @$path;
      foreach my $level (@$path) {
         if (defined (my $h = $self->topics->{$level})) {
            $self = $h;
         } elsif ((my @candidates = grep { is_object($_) } map { $_->category && $_->topics->{$level} } values %{$self->topics})) {
            if (@candidates > 1) {
               croak( "ambiguous help path matching following topics: ", join(", ", map { $_->full_path } @candidates));
            }
            $self = $candidates[0];
         } else {
            push @{$self->toc}, $level;
            $self = $self->topics->{$level} = new Help($self, $level);
            $self->defined_at = "$source_file, line $source_line";
         }
      }

      # some topics automatically come into existence before the description is encountered in the rules
      my $h;
      if ($cat) {
         $self->category = 1;

         if (length($self->text) == 0) {
            # the category has not been introduced so far, let's look in the global items...
            $top //= $self->top;
            my @app_being_loaded;
            if (is_object($INC[0]) && instanceof Application($INC[0]) && $INC[0]->help != $top) {
               push @app_being_loaded, $INC[0]->help;
            }
            my ($specific_cat_group, $any_node, $matching_cat);
            foreach my $app_top_help ($top, @app_being_loaded, @{$top->related}) {
               if ($specific_cat_group = $app_top_help->topics->{$self->parent->name}
                     and
                   $any_node = $specific_cat_group->topics->{any}
                     and
                   $matching_cat = $any_node->topics->{$self->name}) {
                  $self->text = $matching_cat->text;
                  last;
               } elsif ($any_node = $app_top_help->topics->{any}
                          and
                        $matching_cat = $any_node->topics->{$self->name}) {
                  $self->text = $matching_cat->text =~ s/\Q+++\E/$self->parent->name/re;
                  last;
               }
            }
            if (length($self->text) == 0) {
               croak( "undocumented category ", $self->full_path );
            }
         }

         if (defined($h = delete $self->parent->topics->{$topic})) {
            my $old_toc = $self->parent->toc;
            for (my ($i, $l) = (0, $#$old_toc); $i <= $l; ++$i) {
               if ($old_toc->[$i] eq $topic) {
                  splice @$old_toc, $i, 1;
                  last;
               }
            }
            $self->topics->{$topic} = $h;
            weak($h->parent = $self);
            push @{$self->toc}, $topic;
         }
      }

      if ($h //= $self->topics->{$topic}) {
         if (!defined($signature) || $signature eq "category") {
            if (defined $text) {
               if (length($h->text)) {
                  $h->text .= $separator . $text if index($h->text,$text)<0;
               } else {
                  $h->text = $text;
               }
            }
            return $h;
         }
         if (my $ovcnt = $h->annex->{function}++) {
            $self = $h;
            ++$ovcnt;
            $topic = "overload#$ovcnt";
         } else {
            $self = $self->topics->{$topic} = new Help($self, $topic);
            $h->name="overload#0";
            $self->topics->{$h->name} = $h;
            weak($h->parent = $self);
            $self->text = $h->text;
            $self->annex->{function} = delete $h->annex->{function};
            $topic = "overload#1";
         }
         delete $annex{function};
      } else {
         push @{$self->toc}, $topic;
      }

      $self = ($self->topics->{$topic} = new Help($self, $topic, $text));
      $self->category = $signature eq "category";
      $self->defined_at = "$source_file, line $source_line";

   } else {
      if (length($self->text)) {
         $self->text .= $separator . $text;
      } else {
         $self->text = $text;
      }
   }

   $self->annex = \%annex;
   if (exists $annex{options}) {
      foreach my $opt_topic (@{$annex{options}}) {
         # connect private option groups to this help topic
         unless (defined($opt_topic->parent)) {
            weak($opt_topic->parent = $self);
            $opt_topic->defined_at = $self->defined_at;
         }
      }
   }
   if (defined(my $fields = delete $annex{fields})) {
      foreach (@$fields) {
         my $field_topic = add($self, [ "methods", $_->name ], $_->text);
         $field_topic->annex->{return} = new ReturnValue($_->type, "data member");
      }
   }
   push @{$self->related}, @related;
   $self;
}
#################################################################################
sub add_tparams {
   my $self = shift;
   $self->annex->{mandatory_tparams} = shift;
   $self->annex->{tparam} //= [ map { new Param($_, undef, undef) } @_ ];
}
#################################################################################
# for functions
sub write_function_text {
   my ($self, $parent, $writer, $full) = @_;
   my ($tparams, $min_tparams, $params, $options, $mandatory, $ellipsis, $return, $depends, $examples) =
      @{$self->annex}{qw(tparam mandatory_tparams param options mandatory ellipsis return depends examples)};

   my $header = ($parent // $self)->name;
   my @visible_tparams = defined($tparams) ? grep { @$_ > 1 } @$tparams : ();
   if ($full) {
      if (@visible_tparams) {
         $header .= "<" . join(", ", map { $_->name } @$tparams) . ">";
      }
   } elsif ($min_tparams > 0) {
      $header .= "<" . join(", ", map { $_->name } @$tparams[0..$min_tparams-1]) . ">";
   }
   $header .= "(";
   if (defined($params)) {
      if (defined($mandatory)) {
         $header .= join(", ", map { $_->name } @{$params}[0..$mandatory])
                  . "; " . join(", ", map { $_->name } @{$params}[$mandatory+1 .. $#$params]);
         if ($ellipsis) {
            $header .= " ... ";
         }
         if (defined $options) {
            $header .= ", " if $mandatory < $#$params;
            $header .= "Options";
         }
      } else {
         $header .= join(", ", map { $_->name } @$params);
      }
   } elsif (defined $options) {
      $header .= "Options";
   }
   $header .= ")";
   if (defined($return)) {
      $header .= " -> " . $return->type;
   }
   $writer->header($header."\n");

   if ($full) {
      write_spez($self, $writer);
      $writer->function_full($self->text || $parent->text);

      if (@visible_tparams) {
         $writer->type_params(@visible_tparams);
      }
      if (defined $params) {
         $writer->function_args($params);
      }
      if (defined $options) {
         foreach my $opt_topic (@$options) {
            $writer->function_options($opt_topic->text, map { @{$_->annex->{keys}} } $opt_topic, @{$opt_topic->related});
         }
      }
      $writer->function_return($return);

      if (defined $depends) {
         $writer->depends($depends);
      }
      if (defined $examples) {
         $writer->examples($examples);
      }

   } else {
      my @options;
      if (defined $options) {
         foreach my $opt_topic (@$options) {
            push @options, map { $_->name } map { @{$_->annex->{keys}} } $opt_topic, @{$opt_topic->related};
         }
      }
      $writer->function_brief(@options);
   }
}

sub display_text {
   my $self = shift;
   my $full;
   if (@_) {
      $full = shift;
      $_[0] = !$full && grep { defined } @{$self->annex}{qw(function depends examples)};
   } else {
      $full = 1;
   }
   my $writer = new PlainText();
   write_text($self, $writer, $full);
   $writer->text
}

sub write_text {
   my ($self, $writer, $full) = @_;
   if (defined(my $ovcnt = $self->annex->{function})) {
      if ($ovcnt) {
         for (0..$ovcnt) {
            $writer->add_separator($separator) if $_ > 0;
            $self->topics->{"overload#$_"}->write_function_text($self, $writer, $full);
         }
      } else {
         $self->write_function_text($self, $writer, $full);
      }
   } else {
      if (length(my $header = $self->annex->{header})) {
         $writer->header($header);
      }
      write_spez($self, $writer);
      $writer->description($self->text);
      if (defined(my $tparams = $self->annex->{tparam})) {
         if (defined($tparams) and my @visible_tparams = grep { defined($_->text) } @$tparams) {
            $writer->type_params(@visible_tparams);
         }
      }
      if (exists $self->annex->{keys}) {
         my $keys;
         $writer->topics_keys(map { defined($keys = $_->annex->{keys}) ? @$keys : () } $self, @{$self->related});
      }

      if ($full) {
         if (defined(my $depends = $self->annex->{depends})) {
            $writer->depends($depends);
         }
         if (defined(my $examples = $self->annex->{examples})) {
            $writer->examples($examples);
         }
      }
   }
}

sub write_spez {
   my ($self, $writer) = @_;
   if (defined (my $spez = $self->annex->{spez})) {
      $writer->specialized(" Only defined for [[" . $spez->name . "]].\n");
   }
}

#################################################################################
sub descend {
   my $self = shift;
   foreach (@_) {
      $self = $self->topics->{$_} or return;
   }
   $self
}

sub get_topics {
   my ($self, $path) = @_;
   return $self if !length($path);
   $path =~ s'^/'';
   my @path = split m'/', $path;
   map { $_->descend(@path) } $self, @{$self->related};
}
#################################################################################
sub list_shallow_matching {
   my ($self, $re, @subtopics) = @_;
   grep { $_->name =~ $re && $_->name ne "any" }
   map { $_->category ? values(%{$_->topics}) : $_ }
   map {
      if (defined (my $topic = $self->topics->{$_})) {
         values(%{$topic->topics})
      } else {
         ()
      }
   } @subtopics
}

sub list_matching_leaves {
   my ($self, $test) = @_;
   map { ( @{$_->toc} ? list_matching_leaves($_, $test) : (),
           !$_->category && $test->($_) ? $_ : () )
   } values %{$self->topics};
}

sub list_matching_children {
   my ($self, $prefix) = @_;
   @{$self->topics}{ grep { index($_, $prefix) == 0 && $_ ne "any" } @{$self->toc} };
}

sub list_matching_topics {
   my $self = shift;
   my $prefix = pop;
   my $norel = shift if $_[0] eq "!rel";
   map {
      if (@_) {
         list_shallow_matching($_, qr/^$prefix/, @_);
      } else {
         ( list_matching_children($_, $prefix),
           list_matching_leaves($_, sub { length($_->text) && index($_->name, $prefix)==0 })
         )
      }
   } $norel ? $self : ($self, @{$self->related})
}

sub list_completions {
   map { $_->name } &list_matching_topics
}

sub find_in_topic {
   my $self = shift;
   my $what = pop;
   @_ ? list_shallow_matching($self, qr/^$what$/, @_)
      : ( @{$self->toc} ? list_matching_leaves($self, sub { $_->name eq $what }) : (),
          !$self->category && $self->name eq $what ? $self : () )
}

sub find {
   my $self = shift;
   my $norel = shift if $_[0] eq "!rel";
   my $opt = shift if $_[0] eq "?rel";
   my @topics = find_in_topic($self, @_);
   if ($opt ? !@topics : !$norel) {
      push @topics, map { find_in_topic($_, @_) } @{$self->related};
   }
   wantarray ? @topics : $topics[0]
}
#################################################################################
sub get_examples {
   my ($self) = @_;
   if (defined(my $examples = $self->annex->{examples})) {
      @$examples
   } elsif (my $ovcnt = $self->annex->{function}) {
      map { get_examples($self->topics->{"overload#$_"}) } 0..$ovcnt
   } else {
      ()
   }
}
#################################################################################
sub map_reduce_for_function {
   my ($self, $map, $reduce) = @_;
   if (my $ovcnt = $self->annex->{function}) {
      my @result;
      foreach my $cnt (0..$ovcnt) {
         local $_ = $self->topics->{"overload#$cnt"}->annex;
         if (my @partial = &$map) {
            if (@result && defined($reduce)) {
               $reduce->(\@result, @partial);
            } else {
               push @result, @partial;
            }
         }
      }
      @result
   } else {
      local $_ = $self->annex;
      &$map
   }
}

sub return_type {
   map_reduce_for_function($_[0], sub {
      if (defined(my $ret = $_->{return})) {
         $ret->type
      } else {
         undef
      }
   }, sub {
      $_[0]->[0] //= $_[1]
   })
}
#################################################################################
sub new_specialization {
   my ($self, $spez)=@_;
   my $spez_topic=$self->find("specializations", $spez->name) //
                  $self->add([ "specializations", $spez->name ], "Unnamed ".($spez->full_spez_for ? "full" : "partial")." specialization of ".$self->name."\n");
   new Specialization($self, $spez_topic);
}
#################################################################################
sub find_type_topic {
   my ($top, $proto, $whence, $force) = @_;
   my $full_name = $proto->full_name;
   my $topic = $top->find($whence, $full_name) //
               (defined($proto->generic)
                ? ($force ? croak( "internal error: asked to create a help node for a parametrized type instance" )
                          : $top->find($whence, $proto->generic->name))
                : ($force ? $top->add([$whence, $full_name])
                          : return));
   if (!exists $topic->annex->{type}) {
      weak($topic->annex->{type} = $proto);
      push @{$topic->related},
           uniq( map { ($_, @{$_->related}) } grep { defined and $_ != $topic and !defined($_->spez_topic) } map { $_->help_topic }
                 $whence eq "property_types" ? defined($proto->super) ? $proto->super : () : @{$proto->super} );

      if (defined($proto->params) && $proto->abstract) {
         $topic->annex->{mandatory_tparams} = $proto->pkg->_min_params;
         my $tparam_docs = $topic->annex->{tparam};
         foreach my $type_param (@{$proto->params}) {
            if (defined($tparam_docs)
                and my ($tparam_descr) = grep { $_->name eq $type_param->name} @$tparam_docs) {
               $tparam_descr->text //= induced_tparam_description($top, $proto, $whence, $type_param->name);
            } elsif (my ($tparam_text) = induced_tparam_description($top, $proto, $whence, $type_param->name)) {
               push @{$topic->annex->{tparam}}, new Param($type_param->name, undef, $tparam_text);
            }
         }
      }
   }
   $proto->help = $topic;
}

# look for any/tparam topics or a description inherited from a super class
sub induced_tparam_description {
   my ($self, $proto, $whence, $param_name) = @_;
   my $topic;
   if (($topic) = $self->get_topics("any/tparam/" . $param_name)
       or
       ($topic) = $self->get_topics("$whence/any/tparam/" . $param_name)) {
      $topic->text
   } elsif (my ($super, $param) = $proto->find_super_type_param($param_name)) {
      map { $_->text } grep { $_->name eq $param_name } @{$super->help_topic->annex->{tparam} // []}
   } else {
      ()
   }
}
#################################################################################

activate(new Topic(undef, "core"));

require Polymake::Core::Help::PlainText;

#################################################################################
package Polymake::Core::Help::Topic::Example;

use Polymake::Struct (
   [ new => '$$$@' ],
   [ '$body' => '#1' ],
   [ '$source_file' => '#2' ],
   [ '$source_line' => '#3' ],
   [ '@hints' => '@' ],
);

#################################################################################
package Polymake::Core::Help::Topic::Param;

use Polymake::Struct (
   [ new => '$$$;$' ],
   [ '$name' => '#1' ],
   [ '$type' => '#2' ],
   [ '$text' => '#3' ],
   [ '$values' => 'undef' ],
   [ '$completer' => 'undef' ],
   [ '$completer_tag' => 'undef' ],
);

sub new {
   my $self = &_new;
   if (length(my $hints = $_[3]) > 0) {
      while ($hints =~ s/^\s*$hint_re\s*//so) {
         my $hint = $1;
         if ($hint =~ /^\s* complete \s+ ($qual_id_re) (?: \( ($id_re) \) )? \s*$/xo) {
            $self->completer = find Shell::Completion::Param($1) // croak( "unknown parameter completer $1" );
            $self->completer_tag = $2;
         } else {
            croak( "unknown parameter hint $hint" );
         }
      }
   }
   $self
}

#################################################################################
package Polymake::Core::Help::Topic::ParamValue;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$value' => '#1' ],
   [ '$text' => '#2' ],
);

#################################################################################
package Polymake::Core::Help::Topic::ReturnValue;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$type' => '#1' ],
   [ '$text' => '#2' ],
);

#################################################################################
package Polymake::Core::Help::Topic::Specialization;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$generic_topic' => '#1' ],
   [ '$spez_topic' => '#2' ],
);

sub add {
   my $self = shift;
   my $topic = $self->generic_topic->add(@_);
   $topic->annex->{spez} = $self->spez_topic;
   $topic
}

sub find { () }
sub list_completions { () }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
