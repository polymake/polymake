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
use feature 'state';
use vars qw( $dirmode $group $permmask $root $ext_root $buildroot $config_file %ConfigFlags $buildmode );
use POSIX qw ( :fcntl_h read write lseek );
use File::Path;
use Config;

use Getopt::Long qw( GetOptions :config require_order no_ignore_case );
unless (GetOptions( 'root=s' => \$root,
                    'extroot=s' => \$ext_root,
                    'buildroot=s' => \$buildroot,
                    'config=s' => \$config_file,
                    'mode=s' => \$buildmode,
                    'group=s' => \$group,
                    'perms=i' => \$permmask ) &&
        defined($config_file) &&
        defined($root) &&
        defined($buildroot) &&
        defined($buildmode) &&
        -f $config_file &&
        -d $root &&
        -d "$buildroot/$buildmode" &&
        (!defined($ext_root) || -d $ext_root)) {
   die <<".";
usage: $0 --root PATH --buildroot PATH --config PATH/config.ninja [ --extroot PATH --extconfig PATH/config.ninja ]
          --buildmode Opt|Debug [ --group GROUP ] [ --perms MASK ] [ shared_module ... ]
.
}

if (defined $group) {
   my $gid=getgrnam($group)
     or die "$0: unknown group '$group'\n";
   $group=$gid;
}

do "$root/support/install_utils.pl";
die $@ if $@;
%ConfigFlags=load_config_file($config_file, $root);
my @BundledExts= $ConfigFlags{BundledExts} =~ /(\S+)/g;

my @default_exclude_for_copy=qw( \. \.\. \.\#.* .*~ \.git.* \.noexport );

# these variables may be used in sourced extension scripts
use vars qw($InstallTop $InstallInc $InstallArch $InstallBin $InstallLib $InstallDoc $builddir $ExtTop $ExtArch);

foreach (qw(InstallTop InstallInc InstallArch InstallBin InstallLib InstallDoc)) {
   no strict 'refs';
   ${$_}=$ConfigFlags{$_};
}
$builddir="$buildroot/$buildmode";
my $ext_name= $ext_root && basename($ext_root);

my $destdir = $ENV{DESTDIR} // $ConfigFlags{DESTDIR};
if (length($destdir)) {
   foreach ($InstallTop, $InstallInc, $InstallArch, $InstallBin, $InstallLib, $InstallDoc) {
      substr($_,0,0).="$destdir/";
   }
}

if (defined $permmask) {
   if (($permmask & 0700) != 0700) {
      die "--perms must allow full access to the file owner\n";
   }
   umask 0;
   $dirmode=0777 & $permmask;
} else {
   $permmask=0777&~umask;
}

if (defined $ext_root) {
   install_extension();
} else {
   install_core();
}

sub install_core {
   my $perlxpath="perlx/$Config::Config{version}/$Config::Config{archname}";

   make_dir($InstallTop);
   foreach my $subdir (qw(demo perllib resources scripts xml)) {
      copy_dir("$root/$subdir", "$InstallTop/$subdir", clean_dir => 1);
   }

   my $clean_dir=1;
   foreach my $file (qw(configure.pl.template generate_applib_fake.pl generate_ninja_targets.pl
                        generate_ninja_targets.pl.template groom_wrappers.pl
                        guarded_compiler.pl install.pl install.pl.template install_utils.pl rules.ninja)) {
      copy_file("$root/support/$file", "$InstallTop/support/$file", clean_dir => $clean_dir);
      $clean_dir=0;
   }

   make_dir("$InstallTop/apps", clean_dir => 1);
   make_dir("$InstallTop/bundled", clean_dir => 1);

   foreach my $bundled (glob("$root/bundled/*")) {
      my $ext_file="$bundled/polymake.ext";
      copy_file($ext_file, $InstallTop.substr($ext_file, length($root)), clean_dir => 1);
      if (-d (my $scripts="$root/bundled/scripts")) {
         copy_dir($scripts, $InstallTop.substr($scripts, length($root)));
      }
   }

   foreach my $dir (glob_all_apps("{perllib,rules,scripts}")) {
      copy_dir($dir, $InstallTop.substr($dir, length($root)), clean_dir => 1);
   }

   make_dir("$InstallInc/polymake", clean_dir => 1);
   copy_dir("$root/lib/core/include", "$InstallInc/polymake",
            exclude => [qw(glue.h Ext.h cout_bridge.h)],
            wrappers => "$root/include/core-wrappers/polymake"
           );
   copy_dir("$root/lib/callable/include", "$InstallInc/polymake");
   copy_dir("$root/lib/core/skel", "$InstallTop/lib/core/skel", clean_dir => 1);

   foreach my $dir (glob_all_apps("include")) {
      my ($app_name)= $dir =~ m{/apps/(\w+)/};
      my $wrappers="$`/include/app-wrappers/polymake/$app_name";
      copy_dir($dir, "$InstallInc/polymake/$app_name",
               -e $wrappers ? (wrappers => $wrappers) : ());
   }

   foreach my $ext_dir ($ConfigFlags{ExternalHeaders} =~ /(\S+)/g) {
      copy_dir("$root/include/external/$ext_dir/$ext_dir", "$InstallInc/polymake/external/$ext_dir",
               clean_dir => 1);
   }

   foreach my $dir (glob_all_apps("src")) {
      copy_dir($dir, $InstallTop.substr($dir, length($root)),
               exclude => [ 'perl' ]);
   }

   make_dir($InstallArch);
   -d $InstallBin || make_dir($InstallBin);
   install_bin_scripts();

   copy_file($config_file, "$InstallArch/config.ninja",
             transform => \&transform_core_config_file);

   make_dir("$InstallArch/bundled", clean_dir => 1);
   foreach my $bundled (@BundledExts) {
      make_dir("$InstallArch/bundled/$bundled");
   }

   -d $InstallLib || make_dir($InstallLib);
   foreach my $lib_file (@ARGV) {
      if ($lib_file =~ m{($perlxpath/auto/.*\.$Config::Config{dlext})$}o) {
         copy_file($lib_file, "$InstallArch/$1", mode => 0555, clean_dir => 1);
      } elsif (-l $lib_file) {
         copy_link($lib_file, "$InstallLib/".basename($lib_file));
      } else {
         copy_file($lib_file, "$InstallLib/".basename($lib_file), mode => 0555);
      }
   }

   make_dir("$InstallArch/lib", clean_dir => 1);
   foreach my $app_dir (glob("$root/apps/*")) {
      my $app_mod=basename($app_dir).".$Config::Config{dlext}";
      copy_file("$builddir/lib/$app_mod", "$InstallArch/lib/$app_mod", mode => 0555);
   }

   if (-f "$buildroot/doc/index.html") {
      copy_dir("$buildroot/doc", $InstallDoc, clean_dir => 1);
   }

   foreach my $bundled (@BundledExts) {
      if (-f (my $inst_script="$root/bundled/$bundled/support/install.pl")) {
         do $inst_script;
         die "installation script $inst_script failed: $@" if $@;
      }
   }
}

sub install_extension {
   $ExtTop="$InstallTop/ext/$ext_name";
   copy_file("$ext_root/polymake.ext", "$ExtTop/polymake.ext", clean_dir => 1);

   foreach my $subdir (qw(resources scripts xml)) {
      if (-d "$ext_root/$subdir") {
         copy_dir("$ext_root/$subdir", "$ExtTop/$subdir");
      }
   }

   make_dir("$ExtTop/apps");
   foreach my $dir (glob("$ext_root/apps/*/{perllib,rules,scripts}")) {
      copy_dir($dir, $ExtTop.substr($dir, length($ext_root)));
   }

   foreach my $dir (glob("$ext_root/apps/*/include")) {
      my ($app_name)= $dir =~ m{/apps/(\w+)/};
      my $wrappers="$`/include/app-wrappers/polymake/$app_name";
      copy_dir($dir, "$ExtTop/include/polymake/$app_name",
               -e $wrappers ? (wrappers => $wrappers) : ());
   }

   foreach my $dir (glob("$ext_root/apps/*/src")) {
      copy_dir($dir, $ExtTop.substr($dir, length($ext_root)),
               exclude => [ 'perl' ]);
   }

   $ExtArch="$InstallArch/ext/$ext_name";
   copy_file("$buildroot/config.ninja", "$ExtArch/config.ninja", clean_dir => 1,
             transform => \&transform_extension_config_file);

   make_dir("$ExtArch/lib");
   foreach my $app_dir (glob("$ext_root/apps/*")) {
      my $app_mod=basename($app_dir).".$Config::Config{dlext}";
      copy_file("$builddir/lib/$app_mod", "$ExtArch/lib/$app_mod", mode => 0555);
   }

   if (-f (my $inst_script="$ext_root/support/install.pl")) {
      do $inst_script;
      die "installation script $inst_script failed: $@" if $@;
   }
}

sub glob_all_apps {
   my ($pattern)=@_;
   ( glob("$root/apps/*/$pattern"),
     map { glob("$root/bundled/$_/apps/*/$pattern") } @BundledExts )
}

sub copy_file {
   my ($from, $to, %options)=@_;
   if (-e $from) {
      if (!-f $from and !-l $from) {
         die "$0: $from is neither a regular file nor a symbolic link\n";
      }
   } else {
      die "$0: $from does not exist\n";
   }
   if ($options{clean_dir}) {
      make_dir(dirname($to), clean_dir => 1);
   }
   &copy;
}

sub copy_link {
   my ($from, $to)=@_;
   my $target=readlink($from);
   if (-e $to) {
      unlink $to
        or die "can't remove old $to: $!\n";
   }
   symlink $target, $to
     or die "$0: can't create a symbolic link $to -> $target: $!\n";
}

sub compile_pattern {
   my $expr=join('|', map { "(?:^$_\$)" } @_);
   qr/$expr/;
}

sub exclude_pattern_for_copy {
   my ($list)=@_;
   if (defined $list) {
      compile_pattern(@$list, @default_exclude_for_copy);
   } else {
      state $default_pattern=compile_pattern(@default_exclude_for_copy);
   }
}

my $z1024=pack "x1024";

sub copy {
   my ($from, $to, %options)=@_;
   my $concat;
   my $mode=$options{mode};

   if (-e $to) {
      if (defined (my $conflict=$options{conflict})) {
         foreach my $c (@$conflict) {
            if ($to =~ $c->[0]) {
               if ($c->[1] eq 'abort') {
                  die "$0: can't install $from: file $to already exists\n";
               } elsif ($c->[1] eq 'concat') {
                  $concat=1;
               } elsif ($c->[1] eq 'keep') {
                  return;
               } elsif ($c->[1] ne 'replace') {
                  die "$0: unknown conflict action $c->[1]\n";
               }
               last;
            }
         }
      }
      unless ($concat || unlink $to) {
         die "$0: can't remove old $to: $!\n";
      }
   }
   my ($fmode, $mtime)=(stat $from)[2,9];
   if (defined $mode) {
      # verbatim copy: assuming binary file
      my $in=POSIX::open $from, O_RDONLY;
      defined($in)
         or die "$0: can't read $from: $!\n";
      my $dummy=O_WRONLY+O_CREAT+O_TRUNC; # bug in AutoLoader!
      my $out;
      if ($concat) {
         open $out, ">>$to";
      } else {
         $out=creat $to, 0600;
      }
      defined($out)
         or die "$0: can't create $to: $!\n";

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
   } else {
      # text file: turn on write protection
      $mode=$fmode & 0555;
      local $/;
      open X, "<", $from
         or die "$0: can't read $from: $!\n";
      $_=<X>; close X;
      if (defined (my $transform=$options{transform})) {
	 &$transform;
      }
      open X, ">", $to
         or die "$0: can't create $to: $!\n";
      print X; close X;
   }
   $mode &= $permmask;
   chmod $mode, $to
     or die "can't modify access rights to $to: $!\n";
   if (defined $group) {
      chown -1, $group, $to
        or die "$0: can't change group of $to: $!\n";
   }
   utime $mtime, $mtime, $to
     or die "can't modify mtime of $to: $!\n";
}

sub make_dir {
   my ($dir, %options)=@_;
   my $clean=$options{clean_dir};

   if (-e $dir) {
      if (-d _) {
         if (defined($dirmode) && ((stat _)[2] & 03777) != $dirmode) {
            chmod $dirmode, $dir
               or die "$0: can't change mode of $dir: $!\n";
         }
         if (defined $group and (stat _)[5] != $group) {
            chown -1, $group, $dir
              or die "$0: can't change group of $dir: $!\n";
         }
         if ($clean) {
            my $error;
            if (defined( my $exclude=$options{exclude})) {
               my $exclude_re=compile_pattern(@$exclude, qw(. ..));
               opendir my $D, $dir;
               foreach (readdir $D) {
                  if ($_ !~ $exclude_re) {
                     if (-d "$dir/$_") {
                        File::Path::remove_tree("$dir/_", { error=>\$error });
                        if (@$error) {
                           die "$0: can't remove old files:\n", map { join(": ", %$_)."\n" } @$error;
                        }
                     } else {
                        unlink "$dir/$_"
                          or die "$0: can't remove old $dir/$_: $!\n";
                     }
                  }
               }
            } else {
               File::Path::remove_tree($dir, { keep_root=>1, error=>\$error });
               if (@$error) {
                  die "$0: can't clean $dir:\n", map { join(": ", %$_)."\n" } @$error;
               }
            }
         }
         return;
      }
      unlink $dir
         or die "$0: can't remove old $dir: $!\n";
   }
   File::Path::make_path($dir, { defined($dirmode) ? (mode => $dirmode) : (),
                                 defined($group) ? (group => $group) : () })
      or die "could not create directory $dir: $!\n";
}

sub copy_dir {
   my ($src, $dst, %options)=@_;
   my $wrappers=delete $options{wrappers};
   my $clean=$options{clean_dir};
   my $exclude_re=exclude_pattern_for_copy($options{exclude});

   if (opendir my $S, $src) {
      make_dir($dst, clean_dir => $clean);
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
            copy_dir($src_f, "$dst/$f", %options);
         } elsif ($wrappers && -f "$wrappers/$f") {
	    $next_created ||= make_dir("$dst/next");
	    copy($src_f, "$dst/next/$f", %options);
	    copy("$wrappers/$f", "$dst/$f", %options,
                 transform => sub { s{^#include_next +"(.*)/([^/]+)"}{#include "$1/next/$2"}m });
	 } else {
            copy($src_f, "$dst/$f", %options);
         }
      }
   } else {
      die "$0: can't traverse $src: $!\n";
   }
}

sub install_bin_scripts {
   local $/;
   open S, "$root/perl/polymake"
     or die "can't read $root/perl/polymake: $!\n";
   $_=<S>;
   close S;

   if ($^O eq "darwin" && $ConfigFlags{ARCHFLAGS} =~ /-arch /) {
      s{^\#!\S+}{#!/usr/bin/arch $ConfigFlags{ARCHFLAGS} $^X}s;
   } else {
      s{^\#!\S+}{#!$Config::Config{perlpath}}s;
   }

   my $init_block=<<"---";
   \$InstallTop="$ConfigFlags{InstallTop}";
   \$InstallArch="$ConfigFlags{InstallArch}";
   \$Arch="$ConfigFlags{Arch}";
   \@BundledExts=qw(@BundledExts);
---
   if ($ConfigFlags{FinkBase}) {
      $init_block.="   \@addlibs=qw($ConfigFlags{FinkBase}/lib/perl5);\n";
   }
   s|(^BEGIN\s*\{\s*\n)(?s:.*?)(^\}\n)|$1$init_block$2|m;

   if (-e "$InstallBin/polymake") {
      unlink "$InstallBin/polymake"
        or die "can't remove old polymake script: $!\n";
   }
   open T, ">$InstallBin/polymake"
     or die "can't create $InstallBin/polymake: $!\n";
   print T;
   close T;

   chmod(0555, "$InstallBin/polymake")
     or die "chmod $InstallBin/polymake failed: $!\n";

   # apply similar transformations to the configuration utility

   my $Version=extract_polymake_version($root);

   open S, "$root/perl/polymake-config"
     or die "can't read $root/perl/polymake-config: $!\n";
   $_=<S>;
   close S;

   if ($^O eq "darwin" && $ConfigFlags{ARCHFLAGS} =~ /-arch /) {
      s{^\#!\S+}{#!/usr/bin/arch $ConfigFlags{ARCHFLAGS} $^X}s;
   } else {
      s{^\#!\S+}{#!$Config::Config{perlpath}}s;
   }

   s/=Version(?=;)/=$Version/;
   s/=InstallTop(?=;)/="$ConfigFlags{InstallTop}"/;
   s/=InstallArch(?=;)/="$ConfigFlags{InstallArch}"/;

   if (-e "$InstallBin/polymake-config") {
      unlink "$InstallBin/polymake-config"
        or die "can't remove old polymake-config script: $!\n";
   }
   open T, ">$InstallBin/polymake-config"
     or die "can't create $InstallBin/polymake-config: $!\n";
   print T;
   close T;

   chmod(0555, "$InstallBin/polymake-config")
     or die "chmod $InstallBin/polymake-config failed: $!\n";
}

sub transform_core_config_file {

   s|^\s* root \s*= \K .*|$ConfigFlags{InstallTop}|xm;
   s|^\s* core\.includes \s*= \K .*|-I$ConfigFlags{InstallInc}|xm;

   my $external_includes= $ConfigFlags{ExternalHeaders} =~ /\S/ ? "-I$ConfigFlags{InstallInc}/polymake/external" : "";
   s|^\s* app\.includes \s*= \K .*|$external_includes|xm;

   s|^(?=\s*Arch\s*=)|PERL = $Config::Config{perlpath}\n|m;
   s|^\s* BuildModes \s*= \K .*|$buildmode|xm;
   s|^\s* DESTDIR \s*= .*$||xm;
}

sub transform_extension_config_file {
   
   s|^\s* root \s*= \K .*|$ConfigFlags{InstallTop}|xm;
   s|^\s* extroot \s*= \K .*|$ConfigFlags{InstallTop}/ext/$ext_name|xm;
   s|^\s* app\.includes \s*= \K .*|-I\${extroot}/include \${super.app.includes}|xm;
   s|^\s* BuildModes \s*= \K .*|$buildmode|xm;
}

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
