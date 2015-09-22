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

#################################################################################
#
#  Help class designed for interactive shell
#
package Polymake::Core::InteractiveHelp;

use Polymake::Struct (
   [ new => ';$$$' ],
   [ '$parent' => 'weak( #1 )' ],
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
declare $separator="-----\n";

# Global flag whether the point of definition of every help topic should be stored.
# This is relatively expensive and used only in the scripts generating documentation pages.
declare $store_provenience=0;

sub full_path {
   my $self=shift;
   my $path=$self->name;
   while (defined ($self=$self->parent)) {
      $path=$self->name."/$path";
   }
   $path
}

# for the sake of interchangeability with ObjectType and PropertyType
sub help_topic { shift }

sub top {
   my $self=shift;
   while (defined ($self->parent)) {
      $self=$self->parent;
   }
   $self
}

#################################################################################
my $stripped=qr{ [ \t]* (.*(?<!\s)) [ \t]*\n }xm;

sub add {
   my ($self, $path, $text, $signature)=@_;
   unless (is_ARRAY($path)) {
      $path=[ split m'/', $path ];
   }

   my ($cat, @related, %annex, $top);
   if (defined $text) {
      if ($text =~ /^\# \s* copyright \s+/xsi) {
         # can happen if some undocumented declaration immediately follows the rule file header
         $text = "UNDOCUMENTED\n";
      } else {
         $text =~ s/^\s*\#\s* \@notest \s*$//xmi;

         if ($text =~ s/^\s*\#\s* \@category $stripped//xomi) {
            splice @$path, -1, 0, $1;
            $cat=1;
         }

         if ($text =~ s/^\s*\#\s* \@relates \s+ $stripped//xomi) {
            my $related=$1;
            $top=$self->top;
            @related=map {
               if (my @to_topic= index($_, "/") >= 0 ? $top->get_topics($_) : $top->find($_)) {
                  if (@to_topic>1) {
                     croak( "help tag \@related '$_' refers to more than one topic, please qualify appropriately" );
                  }
                  ($to_topic[0], @{$to_topic[0]->related})
               } else {
                  croak( "help tag \@related '$_' refers to an unknown topic" );
               }
            } split /(?: \s*,\s* | \s+)/x, $related;
         }

         my ($opt_group, @option_lists);
         while ($text =~ s/^\s*\#\s* \@($id_re) [ \t]+ ( .*\n (?:^ (?>\s*\#[ \t]*) (?! \@$id_re) .*\n)* )//xom) {
            my ($tag, $value)=(lc($1), $2);
            sanitize_help($value);

            if ($tag eq "author" or $tag eq "display") {
               $annex{$tag}=$value;

            } elsif ($tag eq "return") {
               if ($value =~ s/^\s* ($type_re) \s*//xos) {
                  $annex{$tag}=[$1, $value];
               } else {
                  croak( "help tag \@return '$value' does not start with a valid type" );
               }
               $annex{function}=0;

            } elsif ($tag eq "tparam") {
               if ($value =~ s/^\s* ($id_re) \s*//xos) {
                  push @{$annex{$tag}}, [$1, $value];
                  $annex{min_tparam} += $value !~ /\bdefault:/;
               } else {
                  croak( "help tag \@tparam '$value' does not start with a valid name" );
               }
               $annex{function}=0;

            } elsif ($tag eq "options") {
               if ($value =~ /^\s* %($qual_id_re) \s*$/xo) {
                  push @option_lists, $1;
               } else {
                  push @{$annex{options}}, $opt_group=[ $value ];
               }
               $annex{function}=0;

            } elsif ($tag eq "param") {
               if ($value =~ s/^\s* (?'type' $type_re) \s+ (?'name' $id_re (?: \s+\.\.\.)?) \s*//xos) {
                  push @{$annex{$tag}}, [$+{type}, $+{name}, $value];
               } else {
                  croak( "help tag \@param '$value' does not start with valid type and name" );
               }
               $annex{function}=0;

            } elsif ($tag eq "value") {
               if ($value =~ s/^\s* (?'name' $id_re) \s+ (?'value' $anon_quoted_re | $id_re ) \s+//xos) {
                  if (defined (my $param_list=$annex{param})) {
                     my ($param)=grep { $_->[1] eq $+{name} } @$param_list
                       or croak( "unknown parameter name '$+{name}'" );
                     push @{$param->[3] //= [ ]}, [$+{value}, $value];
                  } else {
                     croak( "help tag \@value must follow parameter descriptions \@param" );
                  }
               } else {
                  croak( "help tag \@value '$value' does not start with valid name and quoted string or named constant" );
               }

            } elsif ($tag eq "option" || $tag eq "key") {
               if ($value =~ s/^\s* (?'type' $type_re) \s+ (?'name' (?!\d)[\w-]+) \s*//xos) {
                  if ($tag eq "key") {
                     $opt_group=($annex{keys} ||= [ ]);
                  } elsif (!$opt_group) {
                     $annex{options}=[ $opt_group=[ "" ] ];
                  }
                  push @$opt_group, [$+{type}, $+{name}, $value];
               } else {
                  croak( "help tag \@$tag '$value' does not start with valid type and name" );
               }
               $annex{function}=0 if $tag eq "option";

            } elsif ($tag eq "depends") {
               push @{$annex{$tag}}, $value;
            } else {
               croak( "unknown help tag \@$tag" );
            }
         }
         if ($signature =~ /[;{\@%]/) {
            my $mandatory=$`;
            my $cnt=0;
            while ($mandatory =~ m/\G\s* (?: [*\$] (?: \s* (?:,\s*)? | $ )
                                           | $type_re (?: \s*\+ )? (?: \s+ | \s*,\s* | $ ))/xog) { ++$cnt; }
            $annex{mandatory}=$cnt-1;
            $annex{ellipsis}=1 if $signature =~ /\@/;
            unless (@option_lists) {
               @option_lists= ($signature =~ /%($qual_id_re)/go);
            }
         }

         foreach (@option_lists) {
            $top ||= $self->top;
            if (defined (my $opt_topic=$top->find("options", $_))) {
               push @{$annex{options}}, $opt_topic;
            }
         }

         sanitize_help($text);
         if (!$cat && $text !~ /\S/) {
            $text="UNDOCUMENTED\n";
         }
      }
   }

   if (@$path) {
      my $topic=pop @$path;
      foreach (@$path) {
         if (defined (my $h=$self->topics->{$_})) {
            $self=$h;
         } else {
            push @{$self->toc}, $_;
            $self=$self->topics->{$_}=new Help($self, $_);
            if ($store_provenience) {
               $self->defined_at=join(", line ", (caller)[1,2]);
            }
         }
      }

      # some topics automatically come into existence before the description is encountered in the rules
      my $h;
      if ($cat) {
         $self->category=1;

         if (length($self->text)==0) {
            # the category has not been introduced so far, let's look in the global items...
            $top ||= $self->top;
            my ($specific_cat_group, $any_node, $matching_cat);
            foreach my $app_top_help ($top, @{$top->related}) {
               if ($specific_cat_group=$app_top_help->topics->{$self->parent->name} and
                   $any_node=$specific_cat_group->topics->{any} and
                   $matching_cat=$any_node->topics->{$self->name}) {
                  $self->text=$matching_cat->text;
                  last;
               } elsif ($any_node=$app_top_help->topics->{any} and
                        $matching_cat=$any_node->topics->{$self->name}) {
                  $self->text=$matching_cat->text;
                  $self->text =~ s/\Q+++\E/$self->parent->name/e;
                  last;
               }
            }
            if (length($self->text)==0) {
               warn_print("undocumented category ", $self->full_path, " at ", join(", line ", (caller)[1,2]));
            }
         }

         if (defined ($h=delete $self->parent->topics->{$topic})) {
            my $old_toc=$self->parent->toc;
            for (my ($i, $l)=(0, $#$old_toc); $i<=$l; ++$i) {
               if ($old_toc->[$i] eq $topic) {
                  splice @$old_toc, $i, 1;
                  last;
               }
            }
            $self->topics->{$topic}=$h;
            weak($h->parent=$self);
            push @{$self->toc}, $topic;
         }
      }

      if ($h //= $self->topics->{$topic}) {
         unless (defined($signature) && $signature ne "category") {
            if (defined $text) {
               if (length($h->text)) {
                  $h->text .= $separator . $text if index($h->text,$text)<0;
               } else {
                  $h->text=$text;
               }
            }
            return $h;
         }
         if (my $ovcnt=$h->annex->{function}++) {
            $self=$h;
            ++$ovcnt;
            $topic="overload#$ovcnt";
         } else {
            $self=$self->topics->{$topic}=new Help($self, $topic);
            $h->name="overload#0";
            $self->topics->{$h->name}=$h;
            $self->text=$h->text;
            $self->annex->{function}=delete $h->annex->{function};
            $topic="overload#1";
         }
         delete $annex{function};
      } else {
         push @{$self->toc}, $topic;
      }

      $self=($self->topics->{$topic}=new Help($self, $topic, $text));
      $self->category= $signature eq "category";
      if ($store_provenience) {
         $self->defined_at=join(", line ", (caller)[1,2]);
      }
   } else {
      if (length($self->text)) {
         $self->text .= $separator . $text;
      } else {
         $self->text=$text;
      }
   }

   $self->annex=\%annex;
   push @{$self->related}, @related;
   $self;
}
#################################################################################
*clean_text=\&InteractiveCommands::clean_text;

sub cleaned_text {
   my $text=(shift)->text;
   clean_text($text);
   $text;
}

# for functions
sub display_function_text {
   my ($self, $parent, $full)=@_;
   my ($tparams, $params, $options, $mandatory, $ellipsis, $return, $depends)=@{$self->annex}{qw(tparam param options mandatory ellipsis return depends)};

   my $text=($parent // $self)->name;
   if (defined $tparams) {
      $text .= "<" . join(", ", map { $_->[0] } @$tparams) . ">";
   }
   $text .= "(";
   if (defined $params) {
      if (defined $mandatory) {
         $text .= join(", ", map { split /\s+/, $_->[1] } @{$params}[0..$mandatory])
               . "; " . join(", ", map { split /\s+/, $_->[1] } @{$params}[$mandatory+1 .. $#$params]);
         if ($ellipsis) {
            $text .= " ... ";
         }
         if (defined $options) {
            $text .= ", " if $mandatory < $#$params;
            $text .= "Options";
         }
      } else {
         $text .= join(", ", map { split /\s+/, $_->[1] } @$params);
      }
   } elsif (defined $options) {
      $text .= "Options";
   }
   $text .= ")";
   if (defined $return) {
      $text .= " -> $return->[0]";
   }
   $text .= "\n\n";
   
   if ($full) {
      my $comment=$self->text || $parent->text;
      clean_text($comment);
      $text .= $comment;

      if ($tparams // $params) {
         $text .= "\nArguments:\n";
      }
      if (defined $tparams) {
         foreach (@$tparams) {
            $comment=$_->[1];
            clean_text($comment);
            $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
            $text .= "  " . InteractiveCommands::underline($_->[0]) . " $comment";
         }
      }
      if (defined $params) {
         foreach (@$params) {
            $comment=$_->[2];
            clean_text($comment);
            $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
            my ($name)= $_->[1] =~ /^($id_re)/;
            $text .= "  $_->[0] " . InteractiveCommands::underline($name) . " $comment";
            if (defined (my $value_list=$_->[3])) {
               $text .= "    Possible values:\n";
               foreach my $value (@$value_list) {
                  $comment=$value->[1];
                  clean_text($comment);
                  $text .= "      " . $value->[0] . " : $comment";
               }
            }
         }
      }
      if (defined $options) {
         foreach my $opt_group (@$options) {
            if (is_object($opt_group)) {
               $comment=$opt_group->text;
               clean_text($comment);
               $text .= "\nOptions: $comment";
               foreach my $topic ($opt_group, @{$opt_group->related}) {
                  my $keys=$topic->annex->{keys} or next;
                  foreach my $key (@$keys) {
                     $comment=$key->[2];
                     clean_text($comment);
                     $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
                     $text .= "  " . InteractiveCommands::underline($key->[1]) . " => $key->[0] $comment";
                  }
               }
            } else {
               $comment=local_shift($opt_group);
               if (length($comment)) {
                  clean_text($comment);
                  $text .= "\nOptions: $comment";
               } else {
                  $text .= "\nOptions:\n";
               }
               foreach my $opt (@$opt_group) {
                  $comment=$opt->[2];
                  clean_text($comment);
                  $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
                  $text .= "  " . InteractiveCommands::underline($opt->[1]) . " => $opt->[0] $comment";
               }
            }
         }
      }
      if (defined $return) {
         $comment=$return->[1];
         clean_text($comment);
         $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
         $text .= "\nReturns $return->[0] $comment\n";
      }
      if (defined $depends) {
        $text .= "Depends on:\n  ";
        my $i = @$depends;
        foreach (@$depends) { 
          if(@$depends > 1) {$text .= "(";}
          clean_text($_);
          $text .= join(", ", split(/\s+/, $_));
          if($i >= 1 && @$depends > 1) {$text .= ")"};
          if($i > 1 ) {$text .= "  or  "};
          --$i
        }
      $text .= "\n";
      }
   } else {
      # brief
      if (defined $options) {
         $text .= "Options:";
         foreach my $opt_group (@$options) {
            if (is_object($opt_group)) {
               foreach my $topic ($opt_group, @{$opt_group->related}) {
                  my $keys=$topic->annex->{keys} or next;
                  foreach my $key (@$keys) {
                     $text .= " ".$key->[1];
                  }
               }
            } else {
               local_shift($opt_group);
               foreach my $opt (@$opt_group) {
                  $text .= " ".$opt->[1];
               }
            }
         }
         $text .= "\n\n";
      }
   }
   $text
}

sub display_keys_text {
   my ($self)=@_;
   my $text="";
   foreach my $topic ($self, @{$self->related}) {
      my $keys=$topic->annex->{keys} or next;
      foreach my $key (@$keys) {
         my $comment=$key->[2];
         clean_text($comment);
         $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
         $text .= "  " . InteractiveCommands::underline($key->[1]) . " => $key->[0] $comment";
      }
   }
   $text
}

sub display_text {
   my $self=shift;
   if (defined (my $ovcnt=$self->annex->{function})) {
      my $full;
      if (@_) {
         $full=shift;
         $_[0]=!$full;
      } else {
         $full=1;
      }
      if ($ovcnt) {
         join($separator, map { $self->topics->{"overload#$_"}->display_function_text($self,$full) } 0..$ovcnt);
      } else {
         display_function_text($self,$self,$full);
      }
   } else {
      my $text=$self->annex->{header} . $self->text;
      if (length($text)) {
         clean_text($text);
      }
      if (exists $self->annex->{keys}) {
         $text .= "\n" . display_keys_text($self);
      }
      if (defined (my $depends=$self->annex->{depends})) {
         $text .= "Depends on:\n  ";
         my $i = @$depends;
         foreach (@$depends) { 
            if (@$depends > 1) {$text .= "(";}
            clean_text($_);
            $text .= join(", ", split(/\s+/, $_));
            if ($i >= 1 && @$depends > 1) {$text .= ")"};
            if ($i > 1 ) {$text .= "  or  "};
            --$i
         }
      }
      $text
   }
}
#################################################################################
sub descend {
   my $self=shift;
   foreach (@_) {
      $self=$self->topics->{$_} or return;
   }
   $self
}

sub get_topics {
   my ($self, $path)=@_;
   $path =~ s'^/'';
   my @path=split m'/', $path;
   map { $_->descend(@path) } $self, @{$self->related};
}
#################################################################################
sub list_shallow_matching {
   my ($self, $re, @subtopics)=@_;
   grep { $_->name =~ $re }
   map { $_->category ? values(%{$_->topics}) : $_ }
   map {
      if (defined (my $topic=$self->topics->{$_})) {
         values(%{$topic->topics})
      } else {
         ()
      }
   } @subtopics
}

sub list_matching_leaves {
   my ($self, $test)=@_;
   map { ( @{$_->toc} ? list_matching_leaves($_,$test) : (),
           !$_->category && $test->($_) ? $_ : () )
   } values %{$self->topics};
}

sub list_toc_completions {
   my ($self, $prefix)=@_;
   grep { index($_,$prefix)==0 } @{$self->toc};
}

sub list_completions {
   my $self=shift;
   my $prefix=pop;
   my $norel=shift if $_[0] eq "!rel";
   map {
      if (@_) {
         map { $_->name } list_shallow_matching($_, qr/^$prefix/, @_);
      } else {
         ( list_toc_completions($_, $prefix),
           map { $_->name } list_matching_leaves($_, sub { length($_->text) && index($_->name, $prefix)==0 }) )
      }
   } $norel ? $self : ($self, @{$self->related})
}

sub find_in_topic {
   my $self=shift;
   my $what=pop;
   @_ ? list_shallow_matching($self, qr/^$what$/, @_)
      : ( @{$self->toc} ? list_matching_leaves($self, sub { $_->name eq $what }) : (),
          !$self->category && $self->name eq $what ? $self : () )
}

sub find {
   my $self=shift;
   my $norel=shift if $_[0] eq "!rel";
   my $opt=shift if $_[0] eq "?rel";
   my @topics=find_in_topic($self, @_);
   if ($opt ? !@topics : !$norel) {
      push @topics, map { find_in_topic($_, @_) } @{$self->related};
   }
   wantarray ? @topics : $topics[0]
}
#################################################################################
sub argument_completions {
   my ($self, $arg_num, $prefix)=@_;
   my ($param_list, $param, $value_list);

   if (my $ovcnt=$self->annex->{function}) {
      map { argument_completions($self->topics->{"overload#$_"}, $arg_num, $prefix) } 0..$ovcnt;

   } elsif (defined ($param_list=$self->annex->{param}) and
            defined ($param=$param_list->[$arg_num])    and
            defined ($value_list=$param->[3])) {
      if ((my $quote)= $prefix =~ /^$quote_re/o) {
         # accept both kinds of quotes
         grep { /^\Q$prefix\E/ } map { $_->[0] =~ $quoted_re ? $quote.$+{quoted}.$quote : $_->[0] } @$value_list;
      } else {
         grep { /^\Q$prefix\E/ } map { $_->[0] } @$value_list;
      }

   } elsif (defined (my $mandatory=$self->annex->{mandatory})) {
      return if $arg_num <= $mandatory;

      my @matching;
      if (defined (my $options=$self->annex->{options})) {
         foreach my $opt_group (@$options) {
            if (is_object($opt_group)) {
               foreach my $topic ($opt_group, @{$opt_group->related}) {
                  my $keys=$topic->annex->{keys} or next;
                  push @matching, grep { /^\Q$prefix\E/ } map { $_->[1] } @$keys;
               }
            } else {
               local_shift($opt_group);
               push @matching, grep { /^\Q$prefix\E/ } map { $_->[1] } @$opt_group;
            }
         }
      }
      map { "$_=>" } @matching;

   } else {
      ()
   }
}
#################################################################################
# => (min, max)
sub expects_template_params {
   my ($self, $rec)=@_;
   if (defined (my $tparams=$self->annex->{tparam})) {
      ($self->annex->{min_tparam}, scalar @$tparams)
   } elsif (!$rec && defined (my $ovcnt=$self->annex->{function})) {
      my ($min_min, $max_max);
      my $ret;
      foreach (0..$ovcnt-1) {
         if (my ($min, $max)=expects_template_params($self->topics->{"overload#$_"}, 1)) {
            assign_min($min_min, $min);
            assign_max($max_max, $max);
            $ret=1;
         }
      }
      $ret ? ($min_min, $max_max) : ()
   } else {
      ()
   }
}

sub return_type {
   my ($self)=@_;
   if (defined (my $ret=$self->annex->{return})) {
      return $ret->[0];
   } elsif (my $ovcnt=$self->annex->{function}) {
      foreach (0..$ovcnt-1) {
         my $h=$self->topics->{"overload#$_"};
         if (defined ($ret=$h->annex->{return})) {
            return $ret->[0];
         }
      }
   }
   undef
}
#################################################################################
# guess the search context for a function:
# consider the first argument and the return type

sub related_objects {
   my ($self, $func_topic)=@_;
   my ($params, $return)=@{$func_topic->annex}{qw(param return)};
   [ uniq( map { $_, @{$_->related} }
               ( defined($params) ? $self->find("objects", $params->[0]->[0]) : (),
                 defined($return) ? $self->find("objects", $return->[0]) : () ) ) ]
}
#################################################################################

# => length of the path up to the first common ancestor
sub proximity {
   my ($from, $to)=@_;
   my %parents;
   do {
      $parents{$to}=1;
   } while (defined($to=$to->parent));

   my $l=0;
   do {
      return $l if exists $parents{$from};
      ++$l;
   } while (defined($from=$from->parent));

   undef
}

#################################################################################

redefine Help;

1


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
