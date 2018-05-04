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


@conf_vars=qw( JDKHome JNIheaders NativeCFLAGS NativeARCHFLAGS NativeSO ANT JAVACMD );


sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( java jni-headers ant ) }=();
}


sub usage {
   print STDERR <<'.';
  --with-java=PATH          ( ${JAVA_HOME} )     where to find Java compiler and runtime environment
  --with-jni-headers=PATH   ( ${java}/include )  where to find jni.h and other headers for native methods
  --with-ant=PATH           ( find via $PATH )   ant executable (Java building tool)
.
}


sub proceed {
   my ($options)=@_;

   # locate the Java VM interpreter

   my $found_by;
   if (defined (my $java=$options->{java})) {
      if (-d $java) {
         if (-f "$java/bin/java" && -x _) {
            $JAVACMD="$java/bin/java";
         } else {
            die "Value of --with-java option does not look like a JDK top directory: $java/bin/java does not exist.\n";
         }
      } elsif (-f _ && -x _) {
         $JAVACMD=$java;
      } elsif ($java !~ m{/}) {
         $JDKHome=Polymake::Configure::find_program_in_path($java)
           or die "Invalid value of --with-java option: program not found along your PATH.\n";
         $JAVACMD=$java;
      } else {
         die "Invalid value of --with-java option: path does not exist.\n";
      }
   } elsif (defined ($java=$ENV{JAVACMD}) && -f $java && -x _) {
     $found_by="JAVACMD";
     $JAVACMD=$java;

   } elsif (defined ($java=$ENV{JAVA_BINDIR}) && -f "$java/java" && -x _) {
     $found_by="JAVA_BINDIR";
     $JAVACMD="$java/java";

   } elsif (defined ($java=$ENV{JAVA_HOME}) && -f "$java/bin/java" && -x _) {
     $found_by="JAVA_HOME";
     $JAVACMD="$java/bin/java";

   } elsif (defined ($java=$ENV{JDK_HOME}) && -f "$java/bin/java" && -x _) {
     $found_by="JDK_HOME";
     $JAVACMD="$java/bin/java";

   } elsif (defined ($java=$ENV{JAVA_ROOT}) && -f "$java/bin/java" && -x _) {
     $found_by="JAVA_ROOT";
     $JAVACMD="$java/bin/java";

   } else {
     $JDKHome=Polymake::Configure::find_program_in_path("java")
       or die "Can't find the Java run-time interpreter on your machine.\n",
              defined($ENV{JAVACMD})
              ? "Environment variable JAVACMD is set, but it does not point to an executable program\n" :
              defined($ENV{JAVA_HOME})
              ? "Environment variable JAVA_HOME is set, but it does not point to a JDK installation directory\n"
              : "Program `java' not found along your PATH.\n",
              "\nPlease specify a valid location in the option --with-java\n",
              "or disable using Java components completely: --without-java.\n",
              "Please note that in the latter case the visualization interfaces\n",
              "to JReality and JavaView will be disabled permanently.\n";
     $JAVACMD="java";
     $found_by="PATH";
   }
   $JDKHome //= $JAVACMD;
   # check Java version

   my ($java_version)= `$JAVACMD -version 2>&1` =~ /version "([\d.]+)/s;
   Polymake::Configure::v_cmp($java_version, "1.7") >= 0
     or die "Java run-time interpreter $JAVACMD",
            $found_by eq "PATH"
            ? " found along your program PATH"
            : $found_by && " found by environment variable $found_by",
            " reports its version as $java_version, while minimal required is 1.7\n",
            "\nPlease upgrade your Java run-time system or JDK to a modern version,\n",
            $found_by && "or specify a correct location in the option --with-java,\n",
            "or disable using Java components completely: --without-java\n",
            "Please note that in the latter case the visualization interfaces\n",
            "to JReality and JavaView will be disabled permanently.\n";

   # locate JNI headers
   if (defined ($JNIheaders=$options->{'jni-headers'})) {
     -f "$JNIheaders/jni.h"
       or die "Value of --with-jni-headers option does not look like JNI include path: $JNIheaders/jni.h does not exist.\n";
   } else {
     my $path=$JDKHome;
     for (;;) {
       if ($path =~ m|/bin/java$|) {
         if (-f "$`/include/jni.h") {
	   # Oracle package structure
           $JNIheaders="$`/include";
           last;
         }
	 if (-f "$`/../include/jni.h") {
	    # OpenJDK package structure
	    $JNIheaders=Cwd::abs_path("$`/../include");
	    last;
	 }
       } elsif ($path =~ m|/Commands/java$|) {
         # MacOS X naming convention
         if (-f "$`/Headers/jni.h") {
           $JNIheaders="$`/Headers";
           last;
         }
       }
       # this trick works under FreeBSD having a wrapper script for all java commands
       local $ENV{JAVAVM_DRYRUN}=1;
       if (`$path 2>/dev/null` =~ /^JAVA_HOME=(.*)$/m && -f "$1/include/jni.h") {
         $JNIheaders="$1/include";
         last;
       }
       (-l $path and $path=readlink($path))
         or die <<".";
$JAVACMD seems to belong to a pure Java runtime installation:
JNI headers (most prominently jni.h) were not found.
You may specify the location of the jdk using the option --with-java or by
setting the JAVA_HOME environment variable.
If the JNI headers happen to reside at a different location, please specify it
in the option --with-jni-headers, but double-check upfront whether they really
suit the chosen Java environment.
Otherwise, please install a complete JDK, or disable building Java components
completely: --without-java.
Please note that in the latter case the visualization interfaces
to JReality and JavaView will be disabled permanently.
.
     }
   }
   # this is needed for calling ANT
   $JDKHome =~ s{(.*)/(?:bin|Commands)/java$}{$1};

   $ANT=$options->{ant};
   if ($ANT ne ".none.") {
      if (defined $ANT) {
	 Polymake::Configure::check_program($ANT);
      } else {
	 Polymake::Configure::find_program($ANT, "ant")
	   or die "ant utility not found; please install it (together with optional targets, if packaged separately)\n",
                  "Specify its location in the option --with-ant if it is installed at a non-standard location.\n";
      }
      my ($ant_version)= `$ANT -version` =~ /version ([\d.]+)/;
      Polymake::Configure::v_cmp($ant_version, "1.7.1") >= 0
         or die "$ANT reports its version as $ant_version, while minimal required version is 1.7.1\n";
      # Java 10 needs javac with nativeheaderdir instead of javah task which requires ant 1.9.8
      Polymake::Configure::v_cmp($java_version, "10") >= 0 and Polymake::Configure::v_cmp($ant_version, "1.9.8") < 0
         and die "$ANT reports its version as $ant_version, while minimal required version for Java 10 is 1.9.8\n";

      $NativeSO="so";

      # MacOS specific magic
      if ($^O eq "darwin") {
	 $NativeSO="jnilib";
	 if (length($ARCHFLAGS)) {
	    ( $NativeARCHFLAGS=`lipo -info $JAVACMD` ) =~ s/.* are: (.*)$/$1/m;
	    $NativeARCHFLAGS =~ s/(\S+)/-arch $1/g;
	 }
      }

      $NativeCFLAGS = '-I${bundled.java.JNIheaders}';
      if (-f (my $platform_jni_dir=glob("$JNIheaders/*/jni_md.h"))) {
	 $platform_jni_dir =~ m{/([^/]+)/[^/]+$};
	 $NativeCFLAGS .= " -I\${bundled.java.JNIheaders}/$1";
      }
   }

   # report the summary

   return join(", ",
               $JAVACMD ne "java" ? ("java=$JAVACMD") : (),
	       $ANT ne ".none." ?
               ( $ANT ne "ant" ? ("ant=$ANT") : (),
		 "JNI headers at $JNIheaders" ) : ()
	      );
}
