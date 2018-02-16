#  Copyright (c) 1997-2018
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
use warnings qw(FATAL void syntax misc);

package Polymake;

sub min($$) {  $_[0]<$_[1] ? $_[0] : $_[1]  }
sub max($$) {  $_[0]>$_[1] ? $_[0] : $_[1]  }
sub assign_min($$) { !defined($_[0]) || $_[0] > $_[1] and $_[0]=$_[1] }
sub assign_max($$) { !defined($_[0]) || $_[0] < $_[1] and $_[0]=$_[1] }
sub assign_min_max($$$) { !defined($_[0]) ? ($_[0]=$_[1]=$_[2]) : $_[0] > $_[2] ? ($_[0]=$_[2]) : $_[1] < $_[2] && ($_[1]=$_[2]) }

sub list_min {
   my $list=shift;
   my $ret;
   assign_min($ret,$_) for @$list;
   $ret;
}

sub list_max {
   my $list=shift;
   my $ret;
   assign_max($ret,$_) for @$list;
   $ret;
}
####################################################################################
sub list_index {     # \@list, numeric scalar or ref => position or -1
   my ($list, $item)=@_;
   my $i=0;
   foreach my $elem (@$list) {
      return $i if $item==$elem;
      ++$i;
   }
   -1
}
####################################################################################
sub string_list_index {     # \@list, string => position or -1
   my ($list, $item)=@_;
   my $i=0;
   foreach my $elem (@$list) {
      return $i if $item eq $elem;
      ++$i;
   }
   -1
}
####################################################################################
sub delete_from_list {  # \@list, numeric scalar or ref => true if deleted
   my ($list, $item)=@_;
   for (my $i=$#$list; $i>=0; --$i) {
      if ($list->[$i]==$item) {
         splice @$list, $i, 1;
         return 1;
      }
   }
   0
}
####################################################################################
sub delete_string_from_list {   # \@list, string scalar => true if deleted
   my ($list, $item)=@_;
   for (my $i=$#$list; $i>=0; --$i) {
      if ($list->[$i] eq $item) {
         splice @$list, $i, 1;
         return 1;
      }
   }
   0
}
####################################################################################
sub contains {          # \@list, numeric scalar or ref => bool
   my ($list, $item)=@_;
   foreach my $elem (@$list) {
      $elem==$item and return 1;
   }
   return 0;
}
####################################################################################
sub contains_string {           # \@list, scalar => bool
   my ($list, $item)=@_;
   foreach my $elem (@$list) {
      $elem eq $item and return 1;
   }
   return 0;
}
####################################################################################
# "string1", "string2" [, "delimiter" ]
# => -1 (s1==prefix(s2)) / 0 (s1==s2) / 1 (prefix(s1)==s2) / 2 (otherwise)
sub prefix_cmp($$;$) {
   my ($s1, $s2, $delim)=@_;
   my $l=min(length($s1), length($s2));
   if (substr($s1,0,$l) eq substr($s2,0,$l)) {
      my $cmpl=length($s1) <=> length($s2);
      if (defined($delim) && $cmpl) {
         return 2 if substr($cmpl>0 ? $s1 : $s2, $l, length($delim)) ne $delim;
      }
      $cmpl
   } else {
      2
   }
}
####################################################################################
sub uniq {
   my %seen;
   grep { !($seen{$_}++) } @_;
}
####################################################################################
sub sorted_uniq {
   my $i=-1;
   grep { ++$i == 0 || $_[$i-1] ne $_ } @_;
}
####################################################################################
sub num_sorted_uniq {
   my $i=-1;
   grep { ++$i == 0 || $_[$i-1] != $_ } @_;
}
####################################################################################
# \list1, \list2 => boolean
sub equal_lists {
   my ($l1, $l2)=@_;
   my $end=@$l1;
   return 0 if $end!=@$l2;
   for (my $i=0; $i<$end; ++$i) {
       $l1->[$i] == $l2->[$i] or return 0;
   }
   1
}

sub equal_string_lists {
   my ($l1, $l2)=@_;
   my $end=@$l1;
   return 0 if $end!=@$l2;
   for (my $i=0; $i<$end; ++$i) {
       $l1->[$i] eq $l2->[$i] or return 0;
   }
   1
}

# \list1, start_index1, \list2, start_index2 => length of the equal sequence starting at given positions
sub equal_sublists {
   my ($l1, $i1, $l2, $i2)=@_;
   my $end=min(@$l1-$i1, @$l2-$i2)+$i1;
   for (; $i1<$end; ++$i1, ++$i2) {
      $l1->[$i1] == $l2->[$i2] or last;
   }
   $i1-$_[1]
}

sub equal_string_sublists {
   my ($l1, $i1, $l2, $i2)=@_;
   my $end=min(@$l1-$i1, @$l2-$i2)+$i1;
   for (; $i1<$end; ++$i1, ++$i2) {
      $l1->[$i1] eq $l2->[$i2] or last;
   }
   $i1-$_[1]
}

# \list1, \list2 => length of the common prefix
sub equal_list_prefixes {
   equal_sublists($_[0], 0, $_[1], 0);
}

sub equal_string_list_prefixes {
   equal_string_sublists($_[0], 0, $_[1], 0);
}

# \list1, \list2, ... => length of the common prefix of list1 and concatenated other lists
sub equal_list_prefixes2 {
   my $l1=shift;
   my $ret=0;
   foreach my $l2 (@_) {
      my $match=equal_sublists($l1, $ret, $l2, 0);
      $ret+=$match;
      last if $ret==@$l1 || $match<@$l2;
   }
   $ret
}
####################################################################################
sub equal_hashes {
   my ($h1, $h2)=@_;
   if (keys(%$h1) == keys(%$h2)) {
      while (my ($k, $v)=each %$h1) {
         unless (exists $h2->{$k} and defined($v) ? $h2->{$k}==$v : !defined($h2->{$k})) {
            keys %$h1;
            return 0;
         }
      }
      1;
   }
}

sub equal_string_hashes {
   my ($h1, $h2)=@_;
   if (keys(%$h1) == keys(%$h2)) {
      while (my ($k, $v)=each %$h1) {
         unless (exists $h2->{$k} and defined($v) ? $h2->{$k} eq $v : !defined($h2->{$k})) {
            keys %$h1;
            return 0;
         }
      }
      1;
   }
}
####################################################################################
sub enforce_nl($) {
   $_[0].="\n" if substr($_[0],-1) ne "\n";
   $_[0]
}
####################################################################################
declare $dbg_prefix="polymake: ";
declare $err_prefix="$dbg_prefix ERROR: ";
declare $warn_prefix="$dbg_prefix WARNING: ";

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";
STDERR->autoflush;
declare $console=\*STDERR;

sub dbg_print {
   print STDERR $dbg_prefix, @_, substr($_[-1],-1) ne "\n" && "\n";
}
sub err_print {
   print $console $err_prefix, @_, substr($_[-1],-1) ne "\n" && "\n";
}
sub warn_print {
   print $console $warn_prefix, @_, substr($_[-1],-1) ne "\n" && "\n";
}
####################################################################################

sub croak {
   my ($pkg, $file, $line, $sub);
   my $i=0;
   if (is_object($INC[0]) && defined($INC[0]->compile_scope)) {
      do {
         ($pkg, $file, $line, $sub)=caller(++$i);
      } while ($pkg =~ /^Polymake(?:$|::Core::|::Overload\b|::Struct\b)/ || $sub =~ /::check_object_pkg$/ );

   } else {
      my ($app_file, $app_line);
      while ($pkg !~ /^Polymake::User\b/  or  $i==1 && $sub =~ /\bAUTOLOAD$/) {
         ($pkg, $file, $line, $sub)=caller(++$i);
         if (!defined $pkg || $pkg eq "main") {
            # not clear where we came from
            local $Carp::CarpLevel=1;
            &Carp::confess;
         }
         if ($file =~ m{/apps/$id_re/(?:rules|perllib|(testsuite))/}o) {
            if ($1 && $app_line) {
               ($file, $line)=($app_file, $app_line); last;
            } else {
               ($app_file, $app_line)=($file, $line);
            }
         }
         if ($file =~ m{/scripts/run_testcases(?:_oo)?$}) {
            undef $pkg;
         }
      }

      if ($file =~ m{^\(eval|/User.pm$} || $sub eq "(eval)") {
         # compatibility mode or command line script
         die @_, "\n";
      }
   }
   local $_=join("", @_);
   s/(?=\.?$)/ at $file line $line./s;
   die enforce_nl($_);
}

sub beautify_error {
   unless ($DebugLevel) {
      # FIXME: remove this!
      namespaces::temp_disable();
      use re 'eval';
      $@ =~ s/ at \(eval \d+\)(?:\[.*?:\d+\])? line 1(\.)?/$1/g;
      $@ =~ s/, <\$?$id_re> line \d+\.//go;
      $@ =~ s/^(?:Compilation failed in require |BEGIN failed--|BEGIN not safe after errors--).*\n//mg;
      $@ =~ s/\A(?s:(.*?))(?<!\bcalled) at (\S+) (?(?{-f $2})(line \d+)|(?!.))(?:,( near .*))?\.?\n/"$2", $3: $1$4\n/mg;
      $@ =~ s/((".*?", line \d+:).*\n)(?:\2 syntax error near ".*?"\n)/$1/mg;
   }
   $@;
}
####################################################################################
sub sanitize_help {
   $_[0] =~ s/^\s*\#//gm;
   $_[0] =~ s/^\#+\n//gm;
   $_[0] =~ s/^(?:\s*\\|[ \t]+)$//gm;
   $_[0] =~ s/\n{3,}/\n\n/gs;
   $_[0] =~ s/^\s*\n//s;
   $_[0] =~ s/\n{2,}$/\n/s;
   mark_as_utf8string($_[0]);
}
####################################################################################
# this is a reduced version of Symbol::delete_package which does not delete the package itself
# used by Configure and ShellHelpers
sub wipe_package {
   my ($stash)=@_;
   foreach my $name (keys %$stash) {
      undef *{$stash->{$name}};
   }
   %$stash=();
}
####################################################################################

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
