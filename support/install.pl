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
use vars qw( $makedir $usage $strip $exclude_re
             $translate_re $translate_back_re $copylink_re @conflict
             $Unlink $modemask $group $convert_next
           );
use POSIX qw ( :fcntl_h read write lseek );
use Config;

$usage=<<".";
usage: $0 [ -m MODE ] [ -g GROUP ] [ -s | -W DIR ]
          [ -{X,L} pattern ... ] [ -T old:new ] [ -U | -C pattern:{abort|concat|keep|replace} ]
          SOURCE TARGET
   or: $0 [ -m MODE ] [ -s ] SOURCE_FILE ... TARGET_DIRECTORY
   or: $0 
   or: $0 -d [ -m MODE ] [ -U ] DIRECTORY ...
.

sub check {
   my ($file)=@_;
   if (-e $file) {
      if (!-f $file and !-l $file) {
         warn "$0: $file is neither a regular file nor a symbolic link\n";
         return 0;
      }
   } else {
      warn "$0: $file does not exist\n";
      return 0;
   }
   1;
}

sub basename {
   $_[0]=~m|([^/]+)$|;
   $1;
}

my $z1024=pack "x1024";

sub copy {
   my ($from, $to, $mode)=@_;
   my $concat;
   if (defined $translate_re) {
      my $other_from=$from;
      return 1 if $other_from =~ s/$translate_back_re/$^R/  and  -e $other_from;
      $to =~ s/$translate_re/$^R/;
   }
   if (!$Unlink and -e $to) {
      foreach my $c (@conflict) {
         if ($to =~ $c->[0]) {
            if ($c->[1] eq 'abort') {
               warn "$0: can't install $from: file $to already exists\n";
               return 0;
            } elsif ($c->[1] eq 'concat') {
               $concat=1;
            } elsif ($c->[1] eq 'keep') {
               return 1;
            } elsif ($c->[1] ne 'replace') {
               die "$0: unknown conflict action $c->[1]\n";
            }
            last;
         }
      }
      unless ($concat || unlink $to) {
         warn "$0: can't remove old $to: $!\n";
         return 0;
      }
   }
   if (-l $from) {
      my $target=readlink($from);
      if ($target !~ $copylink_re) {
         unless (symlink $target, $to) {
            warn "$0: can't create $to -> $from: $!\n";
            return 0;
         }
         return 1;
      }
   }
   my ($fmode, $mtime)=(stat $from)[2,9];
   if (defined $mode) {
      # verbatim copy - assuming binary file
      my $in=POSIX::open $from, O_RDONLY;
      if (!defined $in) {
         warn "$0: can't read $from: $!\n";
         return 0;
      }
      my $dummy=O_WRONLY+O_CREAT+O_TRUNC; # bug in AutoLoader!
      my $out;
      if ($concat) {
         open $out, ">>$to";
      } else {
         $out=creat $to, 0600;
      }
      if (!defined $out) {
         warn "$0: can't create $to: $!\n";
         return 0;
      }

      my $trailing_zeroes;
      while ((my $size=read $in, $_, 1024)>0) {
         if ($_ eq ($size==1024 ? $z1024 : pack("x$size"))) {
            lseek $out, $size, SEEK_CUR;
            $trailing_zeroes=1;
         } else {
            write $out, $_, $size;
            $trailing_zeroes=0;
         }
      }
      if ($trailing_zeroes) {
         lseek $out, -1, SEEK_CUR;
         write $out, "\0", 1;
      }
      POSIX::close $in;
      POSIX::close $out;
      system "strip $to" if $strip;
   } else {
      # text file
      $mode=0444;
      unless (open X, "<", $from) {
         warn "$0: can't read $from: $!\n";
         return 0;
      }
      $_=<X>; close X;
      if ($convert_next) {
	 s{^#include_next +"(.*)/([^/]+)"}{#include "$1/next/$2"}m;
      }
      unless (open X, ">", $to) {
         warn "$0: can't create $to: $!\n";
         return 0;
      }
      print X; close X;
      $mode&=$modemask;
   }
   utime $mtime, $mtime, $to
      and
   chmod $mode, $to
      and
   do {
      !defined $group
      or chown -1, $group, $to
      or do {
         warn "$0: can't change group of $to: $!\n";
         0
      }
   }
}

sub make_dir {
   my ($dir, $mode)=@_;
   if (-e $dir) {
      if (-d _) {
         if (defined $mode and ((stat _)[2] & 03777) != $mode) {
            unless (chmod $mode, $dir) {
               warn "$0: can't change mode of $dir: $!\n";
               return 0;
            }
         }
         if (defined $group and (stat _)[5] != $group) {
            unless (chown -1, $group, $dir) {
               warn "$0: can't change group of $dir: $!\n";
               return 0;
            }
         }
         if ($Unlink) {
            opendir my $D, $dir;
            foreach (readdir $D) {
               if ($_ !~ $exclude_re  and  -l "$dir/$_" || -f _) {
                  unless (unlink "$dir/$_") {
                     warn "$0: can't remove old $dir/$_: $!\n";
                     return 0;
                  }
               }
            }
         }
      } else {
         warn "$0: $dir is not a directory\n";
         return 0;
      }
      return 1;
   }
   my @path=split m|/|, $dir;
   my $accumulated=".";
   if ($path[0] eq "") {
      $accumulated="";
      shift @path;
   }
   foreach my $p (@path) {
      $accumulated.="/$p";
      if (-e $accumulated) {
         next if -d _;
         warn "$0: $accumulated is not a directory\n";
         return 0;
      }
      unless (mkdir $accumulated, $mode) {
         # some other process might have created the folder in the meantime
         # and we got a 'file exists' error
         next if -d $accumulated;
         warn "$0: can't create $accumulated: $!\n";
         return 0;
      }
      if (defined $group and (stat $accumulated)[5] != $group) {
         unless (chown -1, $group, $accumulated) {
            warn "$0: can't change group of $accumulated: $!\n";
            return 0;
         }
      }
   }
   1;
}

sub copy_dir {
   my ($src, $dst, $dirmode, $wrappers)=@_;
   if (opendir my $S, $src) {
      make_dir($dst, $dirmode) or return 0;
      my (%noexport, @noexport_patterns, $next_created);
      if (-f "$src/.noexport") {
         open my $noexport, "$src/.noexport"
           or die "can't read $src/.noexport: $!\n";
         local $/="\n";
         while (<$noexport>) {
            if (my ($file, $status)= /^(?!\#)(\S+)\s+(\S+)$/) {
               if ($file =~ /[?*\[\]]/) {
                  $file =~ s/./\\./g;
                  $file =~ tr/?/./;
                  $file =~ s/\*/.*/g;
                  push @noexport_patterns, [ qr{^$file$}, $status ];
               } else {
                  $noexport{$file}=$status;
               }
            }
         }
      }
      foreach my $f (grep { $_ !~ $exclude_re } readdir $S) {
         my $noexport=$noexport{$f};
         unless ($noexport) {
            foreach my $pat (@noexport_patterns) {
               if ($f =~ $pat->[0]) {
                  $noexport=$pat->[1];
                  last;
               }
            }
         }
         if ($noexport) { 
            next if $noexport ne "local";
         }
         my $src_f="$src/$f";
         if (-d $src_f) {
            copy_dir($src_f, "$dst/$f", $dirmode) or return 0;
         } elsif ($wrappers && -f "$wrappers/$f") {
	    $next_created ||= make_dir("$dst/next",$dirmode) or return 0;
	    copy($src_f, "$dst/next/$f") or return 0;
	    local $convert_next=1;
	    copy("$wrappers/$f", "$dst/$f") or return 0;
	 } else {
            copy($src_f, "$dst/$f") or return 0;
         }
      }
      1
   } else {
      warn "$0: can't traverse $src: $!\n";
      0
   }
}

$makedir=$strip=$Unlink=0;
$modemask=0777;
undef $/;
my ($mode, $dirmode, $wrappers, %patterns);

while (@ARGV && $ARGV[0] =~ /^-/) {
   my $opt=shift;
   if ($opt eq "--") {
      last;
   }
   if ($opt eq "-d") {
      $makedir=1;
      next;
   }
   if ($opt eq "-U") {
      $Unlink=1;
      next;
   }
   if ($opt eq "-s") {
      $strip=1;
      next;
   }
   die $usage if !@ARGV;
   if ($opt eq "-m") {
      $mode=oct shift;
      next;
   }
   if ($opt eq "-g") {
      $_=shift;
      defined($group=getgrnam($_))
      or die "$0: unknown group '$_'\n";
      next;
   }
   if ($opt =~ "-[XTLC]") {
      $_=shift;
      s/\./\\./g;  s/\?/./g;  s/\*/.*/g;
      push @{$patterns{$opt}}, $_;
      next;
   }
   if ($opt eq "-W") {
      $wrappers=shift;
      undef $wrappers unless -d $wrappers;
      next;
   }
   die "$0: unknown option: $opt\n$usage";
}

die $usage if $makedir && keys %patterns
           or @ARGV < 2-$makedir;
die "$0: can't strip non-executables\n" if $strip and !defined($mode) || !($mode & 0111);

if ($makedir) {
   if (defined $mode) {
      umask 0;
   } else {
      $mode=0777&~umask;
   }
   map { make_dir($_,$mode) or exit 1 } @ARGV;
} else {
   if (defined $mode) {
      umask 0;
   } else {
      $mode=0666&~umask;
   }

   if (-d $ARGV[0]) {
      die $usage if @ARGV > 2;
      $dirmode=$mode|0200;
      undef $mode;
      $modemask=$dirmode&0111;
      $modemask|=$modemask<<2;
      if ($patterns{'-T'}) {
         use re 'eval';
         my $translate=join('|', map { my ($from,$to)=split /:/; "(?:$from(?{\"$to\"}))" } @{$patterns{'-T'}});
         my $translate_back=join('|', map { my ($from,$to)=split /:/; "(?:$to(?{\"$from\"}))" } @{$patterns{'-T'}});
         $translate_re=qr/$translate/;
         $translate_back_re=qr{(?=[^/]+$)(?:$translate_back)};
      }
      my $copylink=join('|', qw(/), exists $patterns{'-L'} ? @{$patterns{'-L'}} : ());
      $copylink_re=qr/^(?:$copylink)/;
      my @global_exclude=qw( \. \.\. \.\#.* \.svn \.noexport );
      if (exists $patterns{'-X'}) {
         push @global_exclude, @{$patterns{'-X'}};
      }
      if (exists $patterns{'-C'}) {
         die $usage if $Unlink;
         foreach (@{$patterns{'-C'}}) {
            my ($pattern, $action)=split /:(?=\w+$)/
               or die "invalid conflict action $_\n";
            push @conflict, [ qr{$pattern}, $action ];
         }
      }
      if (!$Unlink) {
         push @conflict, [ qr{.} => 'replace' ];
      }

      my $global_exclude=join('|', map { "(?:^$_\$)" } @global_exclude);
      $exclude_re=qr/$global_exclude/;
      copy_dir(@ARGV, $dirmode, $wrappers) or exit 1;
   } elsif (!-e $ARGV[0]) {
      warn "$0: $ARGV[0] skipped non-existing directory\n";
   } else {
      die $usage if $Unlink or exists $patterns{'-X'} or exists $patterns{'-L'} or exists $patterns{'-C'};
      if (-d $ARGV[-1]) {
         my $dst=pop @ARGV;
         if (-w _) {
            foreach my $src (@ARGV) {
               check($src) && copy($src, "$dst/".basename($src), $mode) or exit 1;
            }
         } else {
            die "$0: target directory $dst not writable\n";
         }
      } elsif (@ARGV==2) {
         check($ARGV[0]) && copy(@ARGV,$mode) or exit 1;
      } else {
         die $usage;
      }
   }
}

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
