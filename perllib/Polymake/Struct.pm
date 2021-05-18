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
use mro;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Struct;
use Polymake::Ext;
use Carp;

$Carp::Internal{'Polymake::Struct'}=1;

require "Polymake/regex.pl";

sub find_expr {
   my $index=shift;
   pos($_)=0;
   while (m{\G .*? (\#\#\#<(\d+)>\n)}xsg) {
      return @- if $2==$index;
   }
   ()
}

sub replace_expr {
   local $_=$_[0];
   if (my ($expr_start, $expr_end)=find_expr($_[1])) {
      substr($_[0],$expr_start,$expr_end-$expr_start)="$_[2] ";
   } else {
      $_[0].="$_[2] ###<$_[1]>\n";
   }
}

sub delete_expr {
   local $_=$_[0];
   if (my ($expr_start, $expr_end)=find_expr($_[1])) {
      substr($_[0],$expr_start,$expr_end-$expr_start)="";
      substr($_[0],$expr_start) =~ s/.*?\n//m;
   }
}

sub import {
   shift;                       # drop the own package name
   my $cnt = 0;
   my ($pkg, $file, $line) = caller(0);
   my $constructor_line = $line;
   my $def = UNIVERSAL::can($pkg, ".defined");
   if ($def) {
      my $other_pkg = method_owner($def);
      if ($pkg eq $other_pkg) {
         unless (ref($_[0]) && $_[0]->[0] eq "alt.constructor") {
            croak "package $pkg already declared as Struct at ".&$def.", conflicting declaration";
         }
      } else {
         croak "inheritance from package $other_pkg declared as Struct at ".&$def.
               "\n seems to be established via plain \@ISA assignment and conflicts with Struct declaration";
      }
   }
   my $symtab = get_symtab($pkg);
   my $constructor = "";
   my $constructor_deferred = "";
   my $alt_constructor_name;
   my ($signature, $own_signature, $check_arg, $min_arg, $max_arg, $keyed_args, %keys, $keys_changed,
       $trailing_list, $trailing_arg, $super, $super_symtab, $super_trailing_arg, $redefine, $merger, $merger_changed);
   my $with_namespaces = namespaces::caller_scope();
   my $prologue = "use strict; $with_namespaces\npackage $pkg";
   while (ref($_[0])) {
      if ($_[0]->[0] eq '@ISA') {
         my $isa = shift;
         shift @$isa;
         if (substr($with_namespaces, 0, 3) eq "use") {
            $_ = namespaces::lookup_class_in_caller_scope($symtab,$_) for @$isa;
         }
         no strict 'refs';
         @{"$pkg\::ISA"}=@$isa;
         mro::set_mro($pkg, "c3");

         foreach my $s (@$isa) {
            if (defined(my $super_constructor = UNIVERSAL::can($s, ".constructor"))) {
               $super = $s;
               ($cnt, $constructor, $constructor_deferred, $merger) = $super_constructor->();
               if (!defined($own_signature) && defined(my $super_signature = UNIVERSAL::can($super, ".signature"))) {
                  ($signature, $min_arg, $max_arg, $trailing_arg) = $super_signature->();
                  $trailing_list = defined($trailing_arg);
                  $super_trailing_arg = $trailing_arg;
               }
               if (my $k = UNIVERSAL::can($s, ".keys")) {
                  $keyed_args = true;
                  %keys = %{$k->()};
               }
               $merger &&= [ @$merger ];
               $redefine = 1;
               namespaces::using($pkg, $super) if $with_namespaces !~ /^no/;
               $super_symtab = get_symtab($super);
               last;
            }
         }
         ++$constructor_line;

      } elsif ($_[0]->[0] eq 'alt.constructor') {
         $def or croak( "package $pkg has not yet been declared as Struct, too early to define alternative constructors" );
         my $alt = shift;
         @$alt == 2 or croak( "alternative constructor name must be single" );
         $alt_constructor_name = $alt->[1];
         my $main_constructor = UNIVERSAL::can($pkg, ".constructor");
         ($cnt, $constructor, $constructor_deferred, $merger) = $main_constructor->();
         ($signature, $min_arg, $max_arg, $trailing_arg) = UNIVERSAL::can($pkg, ".signature")->();
         $trailing_list = defined($trailing_arg);
         if (my $k = UNIVERSAL::can($pkg, ".keys")) {
            $keyed_args = true;
            %keys = %{$k->()};
         }
         $redefine = 1;
         ++$constructor_line;

      } elsif ($_[0]->[0] eq 'new') {
         $signature = (shift)->[1];
         if ($signature !~ m'^ (\$*) (?: ;(\$+) | (%))? (\@)? $ 'x) {
            croak "invalid constructor signature: '$signature'";
         }
         $own_signature = $signature;
         $min_arg = length($1);
         $check_arg = $min_arg>0 && "\@_ <= $min_arg";
         $max_arg = $min_arg+length($2)+1;
         $trailing_list = defined($4);
         if (defined($2)) {
            if (!$4) {
               $check_arg .= ($min_arg>0 && " or ") ."\@_ > $max_arg";
            }
         } elsif ($3) {
            $keyed_args = true;
         }
         ++$constructor_line;

      } elsif ($_[0]->[0] eq 'aliases') {
         $redefine && !$alt_constructor_name
           or croak "a Struct-based super class is required to define aliases";
         my $list = shift;
         for (my ($i, $n) = (1, $#$list); $i < $n; $i+=2) {
            my @aliases = split /\s*\|\s*/, $list->[$i];
            my $ref = $list->[$i+1];
            my $accessor = UNIVERSAL::can($super, $ref) or croak "alias declaration refers to an unknown field $ref";
            my $index = get_field_index($accessor);
            $index >= 0 or croak "alias declaration refers to a method $ref instead of a field";
            define_function($symtab, $_, $accessor) for @aliases;
            if (defined(my $k = $keys{$ref})) {
               $keys{$_} = $k for @aliases;
            }
            $keys_changed = true;
         }
         ++$constructor_line;

      } else {
         last;
      }
   }

   foreach my $field (@_) {
      my ($code, $name) = split //, (ref($field) ? $field->[0] : ($alt_constructor_name ? croak( "lacking initializer for $field" ) : undef($redefine), $field)), 2;
      if ($code !~ m'[$=@%&]') {
         croak "unknown field type '$code'$name";
      }
      my @aliases = split /\s*\|\s*/, $name;
      my $accessor = UNIVERSAL::can($pkg, $aliases[0]);
      my $index;
      if (defined($redefine)) {
         if (!defined($accessor) || ($redefine = get_field_index($accessor)) < 0) {
            $alt_constructor_name
              and croak( "unknown field name $name" );
            undef $redefine;    # new field or an occasionaly overwritten non-field method
            $index = $cnt;
         } else {
            $index = $redefine;
         }
      } elsif (get_field_index($accessor) >= 0 && method_owner($accessor) eq $pkg) {
         croak "multiple definition of field $aliases[0]";
      } else {
         $index = $cnt;
      }
      my ($key_in_expr, $set_filter, $set_filter_is_method, $init_deferred, $init_deferred_expr, $merge_expr);
      my $deflt = $code eq '@' ? "[]" :
                  $code eq '%' ? "{}" :
                  $code eq '$' ? "''" :
                                 "undef";
      my $add_to_keys = 0;
      my $init =
         ref($field)
         ? $field->[1] eq '@'
           ? do {
                if ($trailing_list) {
                   croak "unexpected options for the trailer-initialized field $name" if $#$field>1;
                   $add_to_keys = -1;
                   $trailing_arg = $index;
                   $code eq '%'
                   ? $keyed_args
                     ? "Polymake::Struct::merge_options({}, \@trailing_list)"
                     : "Polymake::Struct::merge_options({}, splice \@_, $max_arg)"
                   : $keyed_args
                     ? "\\\@trailing_list"
                     : "[ splice \@_, $max_arg ]"
                } else {
                   croak "no trailing list declared in the signature"
                }
             } :
           $code eq '&' && $field->[1] =~ /^(?:undef|->\s*(\w+))(?:\s*\|\|\s*(.+))?$/
           ? do {
                croak "unexpected options for the method field $name" if $#$field>1;
                $add_to_keys = -1;
                if ($2) {
                   $set_filter = eval "$prologue; $2";
                   croak "syntax error compiling method fallback for the field $name: $@" if $@;
                }
                if (!defined($1)) {
                   "undef"
                } elsif ((my $super_index=get_field_index(UNIVERSAL::can($pkg, "$1"))) >= 0) {
                   unless (exists &{$symtab->{original_object}}) {
                      define_function($symtab, "original_object", \&original_object);
                   }
                   $super_index;
                } else {
                   croak "reference to an unknown field $1 in initializer of the field $name";
                }
             }
           : do {
                croak "odd options list for the field $name" if !($#$field%2);
                my $expr = $field->[1];
                if ($expr =~ s/^\s* weak\s*\( (.*) \)\s*$/$1/x) {
                   $init_deferred_expr = "weak(\$this->[$index]);";
                }
                my %options = splice @$field, 2;
                $init_deferred = $set_filter_is_method = $expr =~ /(?<!\\)\$this\b/;
                if ($expr =~ s{(?: ^ | [\s,([{]) \K \#(?:([\d+]) | (%)) (?= $ | [-\s,)\]}])}
                              { $2 ? $keyed_args
                                     ? ($add_to_keys = 1, '#%')
                                     : croak("constructor has no keyword arguments")
                                   : $1<$max_arg
                                     ? ($add_to_keys = -1, '$_['.($1-$init_deferred).']')
                                     : croak("constructor argument #$1 out of range") }egx) {
                   if ($add_to_keys > 0) {
                      my $get_kname = "\$kw[2*$index]";
                      $key_in_expr = $expr ne "#%";
                      if (defined(my $default_arg = delete $options{default})) {
                         $init_deferred ||= $default_arg =~ /(?<!\\)\$this\b/;
                         if ($key_in_expr) {
                            $deflt = $expr =~ s/\#%/'$aliases[0]', $default_arg/gr;
                         } else {
                            $deflt = $default_arg;
                         }
                      }
                      my $val = "\$kw[2*$index+1]";
                      if ($code eq '@') {
                         $val = "(CORE::ref($val) eq 'ARRAY' ? $val : [$val])";
                      }
                      if ($key_in_expr) {
                         if (defined($merge_expr = delete $options{merge})) {
                            $merge_expr =~ s/\#%/\@_/ and
                            $merge_expr = "$prologue; sub { my \$this=shift; $merge_expr }";
                         }
                         $expr =~ s/\#%/$get_kname, $val/g;
                         $expr = "defined($get_kname) ? ($expr) : Polymake::Struct::mark_as_default($deflt)";
                      } else {
                         $expr = "$get_kname ? $val : Polymake::Struct::mark_as_default($deflt)";
                      }
                   }
                }
                if ($add_to_keys <= 0 && defined($set_filter = delete $options{set_filter})) {
                   unless (is_string($set_filter) || is_code($set_filter)) {
                      croak "set_filter option for the field $name must refer to a sub or a method name";
                   }
                }
                if (keys %options) {
                   croak "unknown or unexpected option(s) ", join(", ", keys %options), " for the field $name";
                }

                if ($code eq '@') {
                   $expr =~ s/^ \s* \( (.*) \) \s* $/[$1]/x
                      or
                   $expr =~ s/^ \s* \@ .*/[$&]/x;
                } elsif ($code eq '%') {
                   $expr =~ s/^ \s* \( (.*) \) \s* $/{$1}/x
                      or
                   $expr =~ s/^ \s* % .*/{$&}/x;
                } elsif ($code eq '=') {
                   if ($key_in_expr) {
                      croak "field $name: global variable reference can't be initialized with a keyword argument";
                   }
                   $expr =~ s/^ \s* ([\w:]+) \s* $/"$1"/x;
                   $init_deferred_expr = "Polymake::Struct::make_alias(\$this, $index);";
                   $add_to_keys = -1;
                } elsif ($code eq '$' && $expr =~ /^\s*(?:new\s+)?$qual_id_re\s*<\s*$id_re/o) {
                   $expr = "($expr)";
                }
                $expr
             }
         : $deflt;

      if ($init_deferred) {
         $init_deferred_expr = "\$this->[$index]=$init;";
         $init = "undef";
      }
      if ($add_to_keys > 0) {
         $keys{$_} = $index for @aliases;
         $keys_changed = true;
      } elsif ($add_to_keys < 0 && defined($redefine) && $keyed_args) {
         delete @keys{@aliases};
         $keys_changed = true;
      }
      $set_filter ||= $key_in_expr && do {
         my $expr = $field->[1];
         if ($expr =~ /^\s* \$this \s*->\s* ((?!\d)\w+) \(\s* \#% \s*\) \s*$/x) {
            $1
         } else {
            my $mys = ($set_filter_is_method &&= " : method") && 'my $this=shift; ';
            $expr =~ s{(?: (?<=^) | (?<=[\s,(])) \#% (?= $ | [\s,)])}{\@_}x;
            eval "$prologue; sub$set_filter_is_method { $mys$expr }"
            or croak "syntax error compiling access filter for the field $name: $@";
         }
      };
      if (defined($redefine)) {
         if ($redefine) {
            my $prev = $redefine-1;
            $constructor =~ /\#\#\#<$prev>\n/s;
            pos($constructor)=$+[0];
         }
         $constructor =~ s/\G .*? (?= ,\s*\#\#\#<$redefine>\n)/$init/xs;

         if (defined($init_deferred_expr)) {
            replace_expr($constructor_deferred, $redefine, $init_deferred_expr);
         } elsif ($constructor_deferred) {
            delete_expr($constructor_deferred, $redefine);
         }
         if ($alt_constructor_name) {
            $set_filter
              and croak( "can't redefine the set filter for field $name" );
         } elsif ($set_filter || defined_scalar($super_symtab->{$aliases[0]})) {
            $accessor=create_accessor($redefine, $code eq '&' ? \&method_call : \&access_field);
            define_function($symtab, $_, $accessor) for @aliases;
            ${$symtab->{$aliases[0]}}=$set_filter;
         }

      } else {
         $constructor .= "$init, ###<$cnt>\n";
         if (defined($init_deferred_expr)) {
            $constructor_deferred .= "$init_deferred_expr ###<$cnt>\n";
         }
         $accessor=create_accessor($cnt, $code eq '&' ? \&method_call : \&access_field);
         define_function($symtab, $_, $accessor) for @aliases;
         if ($set_filter) {
            ${$symtab->{$aliases[0]}}=$set_filter;
         }
         ++$cnt;
      }

      if (defined($merge_expr)) {
         $merger ||= [ ];
         $merger->[$index]=eval $merge_expr;
         if ($@) {
            croak "syntax error compiling merger expression '$merge_expr' for the field $aliases[0]: $@";
         }
         $merger_changed=1;
      }
   }

   if (@_ || defined($own_signature)) {
      if (defined($super_trailing_arg) and !$trailing_list || $trailing_arg!=$super_trailing_arg) {
         if ($super_trailing_arg) {
            my $prev=$super_trailing_arg-1;
            $constructor =~ /\#\#\#<$prev>\n/s;
            pos($constructor)=$+[0];
         } else {
            pos($constructor)=0;
         }
         $constructor =~ s/\G (Polymake::Struct::merge_options\()? .*? (?= ,\s*\#\#\#<$super_trailing_arg>\n)/ $1 ? "{ }" : "[ ]" /xse;
      }
      my ($proc_keys, $post_merge_proc);
      my $constr_name= $alt_constructor_name // "__new";
      my $proc_kw_name= ".process_keywords" . ($alt_constructor_name && ".$alt_constructor_name");
      my $msg_constr_name= $alt_constructor_name // "new";
      my $new_text= <<"_#_0_#_";
$prologue;
sub $constr_name {
_#_0_#_
      if ($keyed_args) {
         $proc_keys=define_function($pkg, $proc_kw_name,
            sub {
               my ($args, $kw, $post_merge, $trailing_list) = @_;
               $#$kw = $cnt-1;
               for (my $i = $max_arg; $i <= $#$args; ++$i) {
                  my $field = $args->[$i];
                  if (is_string($field) && defined (my $k=$keys{$field}) && $i<$#$args) {
                     $kw->[2*$k]=$field; $kw->[2*$k+1]=$args->[++$i];
                  } elsif (is_hash($field)) {
                     while (my ($kn, $kv) = each %{$field}) {
                        if (defined(my $k = $keys{$kn})) {
                           if (defined($post_merge) && defined($merger->[$k]) && defined($kw->[2*$k])) {
                              push @$post_merge, $k, $kn, $kv;
                           } else {
                              $kw->[2*$k]=$kn; $kw->[2*$k+1]=$kv;
                           }
                        } else {
                           local $Carp::CarpLevel=1;
                           my $accessor = UNIVERSAL::can($pkg,$kn);
                           if (defined($accessor) && get_field_index($accessor)>=0) {
                              croak "$pkg\::$msg_constr_name - member $kn can't be initialized with a keyword argument";
                           } else {
                              croak "$pkg\::$msg_constr_name - unknown keyword $kn";
                           }
                        }
                     }
                  } elsif (defined($trailing_list)) {
                     @$trailing_list = splice @$args, $i;
                     last;
                  } else {
                     local $Carp::CarpLevel = 1;
                     my $accessor;
                     croak( "$pkg\::$msg_constr_name - ", 
                            $i==$#$args
                            ? "keyword $field without value" :
                            is_string($field)
                            ? ( defined($accessor = UNIVERSAL::can($pkg,$field)) && get_field_index($accessor)>=0
                                ? "member $field can't be initialized with a keyword argument"
                                : "unknown keyword $field" )
                            : "expected keyword or HASH, got ".(ref($field) || "'$field'") );
                  }
               }
            });
         my $post_merge_arg="";
         if (defined($merger)) {
            $new_text .= "   my \@post_merge;\n";
            $post_merge_arg=",\\\@post_merge";
            $post_merge_proc=define_function($pkg, ".post_merge",
               sub {
                  my $this=shift;
                  for (my $i=0; $i<$#_; $i+=3) {
                     my $k=$_[$i];
                     $this->[$k]=$merger->[$k]->($this,@_[$i+1,$i+2],$this->[$k]);
                  }
               });
         }
         if (defined $trailing_arg) {
            $post_merge_arg ||= ",undef";
            $new_text .= <<"_#_1_#_" ;
   my (\@kw,\@trailing_list);
   \$proc_keys->(\\\@_,\\\@kw$post_merge_arg,\\\@trailing_list);
_#_1_#_
         } else {
            $new_text .= <<"_#_2_#_";
   my \@kw;
   \$proc_keys->(\\\@_,\\\@kw$post_merge_arg);
_#_2_#_
         }
      }
      if ($check_arg) {
         $new_text .= <<"_#_3_#_";
   if ($check_arg) {
      Polymake::croak( "usage: $msg_constr_name ", CORE::ref(\$_[0]) || \$_[0], '($signature)' );
   }
_#_3_#_
      }
      --$constructor_line;
      $new_text .= <<"_#_4_#_";
#line $constructor_line "$file"
_#_4_#_
      if ($constructor_deferred || defined($post_merge_proc)) {
         my $call_post_merge= defined($post_merge_proc) && "   \$post_merge_proc->(\$this,\@post_merge) if \@post_merge;";
         $new_text .= <<"_#_5_#_";
   my \$this=Polymake::Struct::make_body(
$constructor
   shift);
$constructor_deferred
$call_post_merge
   \$this
_#_5_#_
      } else {
         $new_text .= <<"_#_6_#_";
   Polymake::Struct::make_body(
$constructor
   shift);
_#_6_#_
      }
      $new_text .= <<"_#_7_#_";
}
_#_7_#_
      eval $new_text;
      if ($@) {
         my @lines = (undef, split /(?:\#\#\#<\d+>)?\n/, $new_text);      # leading undef makes the line numbers = array index
         $@ =~ s/at \(eval \d+\) line (\d+)/near line $1: '$lines[$1]'/g;
         croak "syntax error in $pkg\::new: $@";
      }
      if ($merger_changed) {
         define_function($symtab, "merge",
            sub : method {
               my $this = shift;
               for (my $i = 0; $i <= $#_; ++$i) {
                  my $field = $_[$i];
                  if (is_string($field) && defined(my $k = $keys{$field}) && $i < $#_) {
                     if (defined($merger->[$k])) {
                        $this->[$k] = $merger->[$k]->($this, $field, $_[++$i], $this->[$k]);
                     } else {
                        $this->$field = $_[++$i];
                     }
                  } elsif (is_hash($field)) {
                     $this->merge(%$field);
                  } else {
                     local $Carp::CarpLevel = 1;
                     croak( "$pkg\::merge - ",
                            $i==$#_ ? "keyword $field without value" :
                            is_string($field) ? "unknown keyword $field" :
                            "expected keyword or HASH, got ".(ref($field) || "'$field'") );
                  }
               }
               $this
            });
      }
   } elsif (!$super) {
      croak "no own fields and no Struct-based super class specified";
   }

   unless ($alt_constructor_name) {
      define_function($symtab, ".defined", sub { "$file line $line" });
      define_function($symtab, ".constructor", sub { ($cnt, $constructor, $constructor_deferred, $merger) });
      define_function($symtab, "sizeof", sub { $cnt });
      define_function($symtab, ".signature", sub { ($signature, $min_arg, $max_arg, $trailing_arg) }) if defined($own_signature);
      if ($keys_changed) {
         define_function($symtab, ".keys", sub { \%keys });
      }
      define_function($symtab, "_new", \&_new);
      if (!$super || UNIVERSAL::can($super, "new")==UNIVERSAL::can($super, "_new")) {
         define_function($symtab, "new", \&_new);
      } elsif ($with_namespaces !~ /^no/) {
         # predeclare for the sake of cleaner syntax in the package's code
         no strict 'refs';
         *{"$pkg\::new"}=UNIVERSAL::can($super, "new");
      }
   }
}

sub complain {
   my ($pkg)=@_;
   Carp::confess( "no constructor for $pkg" );
}

sub _new { &{UNIVERSAL::can($_[0], "__new") // &complain } }

sub merge_options {
   my $hash=shift;
   for (my $i=0; $i<=$#_; ++$i) {
      if (is_hash($_[$i])) {
         push %$hash, %{$_[$i]};
      } else {
         $hash->{$_[$i]}=$_[$i+1];
         ++$i;
      }
   }
   $hash
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
