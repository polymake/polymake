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
####################################################################################
# the following routines are defined for lists of integers sorted in increasing order

# s1, s2, compl2 => length of s1*=s2 (!compl2) or s1-=s2 (compl2)
sub set_intersection {
   my ($s1, $s2, $compl2)=@_;
   my ($i1, $i2, $l1, $l2)=(0,0,scalar(@$s1),scalar(@$s2));
   $l1 or return 0;
   while ($i2<$l2) {
      my $diff= $s1->[$i1] <=> $s2->[$i2];
      ++$i2, next if $diff>0;
      if (!$diff==$compl2) {
         splice @$s1, $i1, 1;
         --$l1;
      } else {
         ++$i1;
      }
      $i1>=$l1 and return scalar(@$s1);
   }
   splice @$s1, $i1;
   scalar(@$s1);
}

# s, elem => length of s-=elem
sub set_remove {
   my ($s, $elem)=@_;
   my $l=@$s;
   for (my $i=0; $i<$l; ++$i) {
      if (my $diff=$s->[$i]<=>$elem) {
         last if $diff>0;
      } else {
         splice @$s, $i, 1;
         return --$l;
      }
   }
   return $l;
}

# s1, s2 => first element of s1*s2 or undef
sub first_set_intersection {
   my ($s1, $s2)=@_;
   my ($i1, $i2, $l1, $l2)=(0,0,scalar(@$s1),scalar(@$s2));
   $l1 or return;
   while ($i2<$l2) {
      my $diff= $s1->[$i1] <=> $s2->[$i2];
      return $s1->[$i1] if !$diff;
      if ($diff>0) {
         ++$i2;
      } else {
         ++$i1>=$l1 and last;
      }
   }
   undef
}

####################################################################################
# \@list, scalar, [order] => bool
# @list must be sorted in increasing order (or decreasing if $order==-1)
sub binsearch($$;$) {
   my ($list, $item, $order)=(@_, 1);
   my ($l, $h)=(0, scalar @$list);
   while ($l<$h) {
      my $m=($l+$h)/2;
      my $cmp=$item <=> $list->[$m];
      return 1 if !$cmp;
      $cmp!=$order ? ($h=$m) : ($l=$m+1);
   }
   return 0;
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
      } while ($pkg =~ /^Polymake(?:$|::Core::|::Overload\b|::Struct\b)/);

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
      }

      if ($file =~ m{^\(eval|/User.pm$} || $sub eq "(eval)") {
         # compatibility mode or command line script
         die @_, "\n";
      }
      if ($file =~ m{/scripts/run_testcases$} && $app_line) {
         ($file, $line)=($app_file, $app_line);
      }
   }
   local $_=join("", @_);
   s/(?=\.?$)/ at $file line $line./s;
   die enforce_nl($_);
}

sub beautify_error {
   unless ($DebugLevel) {
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

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
