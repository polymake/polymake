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

@make_vars=qw( JavaViewPath JavaViewClassPath ANTflags );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( javaview ) }=();
}


sub usage {
  print STDERR "--with-javaview=PATH      where JavaView is installed\n";
}


sub proceed {
   my ($options)=@_;
   my $script;
   if (defined (my $path=$options->{javaview})) {
     if (-f $path && -x _) {
       $script=$path;
     } elsif (-d $path && -f "$path/_jarVersion.txt" && -d "$path/rsrc") {
       $JavaViewPath=$path;
     } else {
       die "Invalid value of --with-javaview option: the path must point to a JavaView installation directory or start script\n";
     }
   } else {
     Polymake::Configure::find_program($script, "javaview")
       or die "Can't find javaview start script along your program PATH.\n",
              "Please specify its location in the option --with-javaview,\n",
              "or disable using JavaView completely: --without-javaview.\n";

   }
   if ($script) {
     $script =~ m|(.*)/[^/]+$|;
     if (-d "$1/../rsrc" && -f "$1/../_jarVersion.txt") {
       # the script is located directly in the installation directory
       $JavaViewPath=Cwd::abs_path("$1/..");
     }
   }

   my ($jv, $parsed);
   local $/;
   if ($JavaViewPath) {
     # can parse the script directly
     open $jv, "$JavaViewPath/bin/javaview"
       or die "can't read the script $JavaViewPath/bin/javaview: $!";
     local $_=<$jv>;
     close $jv;
     if (($JavaViewClassPath)= /^\s*CLASSPATH\s*=\s*(\S+)/m) {
       $JavaViewClassPath =~ s{\$JAVAVIEW_HOME}{$JavaViewPath}g;
       $JavaViewClassPath =~ s{\$JAVAVIEW_JARS}{$JavaViewPath/jars}g;
       $parsed=1;
     }
   } elsif ($^O ne "darwin") {
     # got to fiddle with shell options
     !$ENV{JAVA_HOME} and $Polymake::Bundled::java::JAVACMD =~ m|^(.*)/[^/]+$| and local $ENV{JAVA_HOME}=$1;
     local @ENV{qw( SHELLOPTS PS4 DISPLAY )}=( "xtrace" );
     open $jv, "$script 2>&1 |" or return "script $script does not start: $!";
     local $_=<$jv>;
     close $jv;
     if (m{ ^CLASSPATH=(.*) (?s: .*?) ^exec \s+ \S+ (?<= [\s/]java) .*? CodeBase=(\S+) }xm) {
       $JavaViewPath=Cwd::abs_path($2);
       $JavaViewClassPath=join(":", map { Cwd::abs_path($_) } split /:/, $1);
       $parsed=1;
     }
   }

   $parsed
     or die $JavaViewPath
            ? "The start script $JavaViewPath/bin/javaview is either very old or highly customized,\n".
              "can't extract the CLASSPATH from it.\n"
            : "Can't deduce the location of the JavaView installation directory and the required CLASSPATH\n" .
              "from its start script; please specify the installation directory in the option --with-javaview.\n";

   # check the version

   open $jv, "$JavaViewPath/_jarVersion.txt"
     or die "Can't check the version in $JavaViewPath/_jarVersion.txt: $!";
   my $version=<$jv>;
   close $jv;
   $version =~ /VERSION\s*=\s*[\"\']?([\d.]+)/ and $version=$1;
   while ($version =~ s/(?<!\d)0(?=\d)/0./g) { }
   Polymake::Configure::v_cmp($version, "3.95")
     or die "Version $version is older than required minimum 3.95\n";

   # report the status

   $ANTflags='-Djavaview.path=${JavaViewPath}';
   return $JavaViewPath;
}
