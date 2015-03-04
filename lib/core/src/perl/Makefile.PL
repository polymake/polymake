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

use Cwd;

use vars qw($TOP $SourceDir $CxxOpt $CxxDebug $libxml2Cflags $libxml2Libs);

my @args=splice @ARGV;
foreach (@args) {
   /^(\w+)=(.*)/ and ${$1}=$2;
}

my @sources=map { m{([^/]+)$} } glob "$SourceDir/*.{xs,xxs,PL}";

my $changed=!-f "Makefile";
opendir D, ".";
foreach (readdir D) {
   if (-l $_ and !-f $_) {
      unlink $_ or die "can't remove obsolete link $_: $!\n";
      $changed=1;
   }
}
closedir D;
foreach (@sources) {
   unless (-l $_) {
      symlink "$SourceDir/$_", $_  or  die "can't create link to $_: $!\n";
      $changed=1;
   }
}
exit 0 unless $changed;

open M, ">MANIFEST";
print M <<'.', map { "$_\n" } @sources;
MANIFEST
.
close M;

use ExtUtils::MakeMaker;

sub MY::makefile {
   package MY;
   my $text=shift->SUPER::makefile(@_);
   $text =~ s{^ .* ==> .* <== (?s: .* $ )}{}xmg;
   $text =~ m{\$\(PERLRUN\) +Makefile\.PL};
   substr($text,$+[0],0) .= join("", map { /=/; " $`='$''"} @args);
   $text .= <<'.';
	$(NOECHO) $(MAKE) $(MAKECMDGOALS) GlueObjects="$(GlueObjects)"
.
}

sub MY::postamble {
   return <<'.' . <<"..";
Bootstrap.h : $(SourceDir)/createBootstrap.pl $(C_FILES) $(CC_FILES)
	$(PERL) $^ >$@

.SUFFIXES : .xxs
.PHONY: compile
compile : $(O_FILES) Bootstrap.h $(CC_FILES)
.

XMLfile\$(OBJ_EXT) : INC += $libxml2Cflags
..
}

sub MY::xs_c {
   package MY;
   my $text=(shift)->SUPER::xs_c;
   if ($] >= 5.010 && $] < 5.012) {
      # avoid annoying compiler warnings about const violations
      $text =~ s{(?= *> *\$\*\.xsc)}{| sed -e 's/^\\( *\\)\\(char\\* *file *= *__FILE__;\\)\$\$/\\1 const \\2/'};
   }
   my $copy=$text;
   $copy =~ s/\.xs\b/.xxs/g;
   $copy =~ s/\.c(?=:|$)/.cc/mg;
   if ($] >= 5.010 && $] < 5.012) {
      $copy =~ s{(?<=\| sed -e ')}{s/\\.xxs\\.c"/.cc"/' -e '};
   } else {
      $copy =~ s{(?= *> *\$\*\.xsc)}{| sed -e 's/\\.xxs\\.c"/.cc"/'};
   }
   return $text.$copy;
}

sub MY::c_o {
   package MY;
   my $text=(shift)->SUPER::c_o;
   # remove harmful compilation rules for C++ modules
   $text =~ s{^\.(?:C|c[^.\$]).*?:\n\t.*?\n}{}mg;
   return $text;
}

my (@CCFiles, @CFiles);
foreach (@sources) {
   if (/\.xxs$/) {
      push @CCFiles, "$`.cc";
   } elsif (/\.xs$/) {
      push @CFiles, "$`.c";
   }
}

(my $numvers=$Config::Config{version}) =~ s/\.//g;
my ($optimize, $otherLDflags);
my ($coverage)=grep { /coverage/ } $CxxOpt, $CxxDebug;
if ($coverage) {
   ($optimize = $Config::Config{optimize}) =~ s/-O\d?/$coverage/;
   $optimize =~ s/\S*FORTIFY_SOURCE\S*//;
   $otherLDflags = $coverage;
}

WriteMakefile(
    NAME        => 'Polymake::Ext',
    macro       => { InstallTop => $TOP, SourceDir => $SourceDir, CC_FILES => "@CCFiles" },
    C           => \@CFiles,
    H           => [ map { "\$(InstallTop)/include/core/polymake/perl/$_" } qw(Ext.h constants.h) ],
    LDFROM      => '$(O_FILES) $(GlueObjects)',
    clean       => { FILES => '$(CC_FILES)'.($coverage && ' *.gcno *.gcda') },
    dynamic_lib => { INST_DYNAMIC_DEP => '$(LDFROM)', defined($otherLDflags) ? (OTHERLDFLAGS => $otherLDflags) : () },
    LIBS        => [ $libxml2Libs ],
    DEFINE      => "-DPerlVersion=$numvers -Wno-nonnull".($coverage && ' -DPMCollectCoverage=1'),
    INC         => "-I$TOP/include/core",
    LIB         => "\$(InstallDir)/perlx/$Config::Config{version}",
    MAN3PODS    => { },
    defined($optimize) ? ( OPTIMIZE => $optimize ) : (),
);


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
