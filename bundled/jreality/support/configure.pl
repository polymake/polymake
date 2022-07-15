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

@conf_vars=qw( JoglJars JoglNative JNIarch );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( jogl jogl-native ) }=();
}

sub usage {
   print STDERR "  --with-jogl=DIR  (a copy bundled with jReality)  location of jogl.jar, Java interface to OpenGL\n",
                "  --with-jogl-native=DIR  (",
                ($^O eq "darwin" || $^O eq "linux") ? "a copy bundled with jReality" : "none",
                ")  location of jogl native library\n",
                "  --without-jogl    disable using jogl; jReality will switch to a slow software renderer.\n";
}

sub check_jogl_native_path {
   my $path=shift;
   my $suffix=$^O eq "darwin" ? "jnilib" : $Config::Config{dlext};
   foreach (qw(jogl gluegen-rt)) {
      if (!-f "$path/lib$_.$suffix") {
	 return "$path/lib$_.$suffix";
      }
   }
   undef
}

sub proceed {
   my ($options)=@_;
   if ($options->{jogl} eq ".none.") {
      # status report
      return "pure software renderer";
   }
   if (defined (my $path=$options->{jogl})) {
      if (-d $path) {
	 foreach (qw(jogl gluegen-rt)) {
	    if (!-f "$path/$_.jar") {
	       die "invalid value of option --with-jogl: file $path/$_.jar does not exist\n";
	    }
	 }
	 $JoglJars=$path;
      } else {
	 die "option --with-jogl must point to a directory\n";
      }
   } else {
      $JoglJars="bundled";
   }

   if (defined (my $path=$options->{'jogl-native'})) {
      if (-d $path) {
	 if (defined (my $missing=check_jogl_native_path($path))) {
	    die "invalid value of option --with-jogl-native: file $missing does not exist\n";
	 }
	 $JoglNative=$path;
      } else {
	 die "option --with-jogl-native must point to a directory\n";
      }

   } elsif (exists $options->{jogl}) {
      for my $try (qw( . lib native jni lib/native lib/jni .. ../lib ../native ../jni ../lib/native ../lib/jni )) {
	 unless (defined(check_jogl_native_path("$JoglJars/$try"))) {
	    $JoglNative=Cwd::abs_path("$JoglJars/$try");
	    last;
	 }
      }
      unless ($JoglNative) {
	 die "Can't find out the location of JOGL native libraries.\n",
	     "Please specify it in the otion --with-jogl-native.\n";
      }

   } else {
      $JNIarch= $^O eq "linux" ? 'linux64' :
                $^O eq "darwin" ? 'macosx'
                                : die <<"---";
jReality needs JOGL (Java interface to OpenGL) for better performance of graphic rendering.
"Unfortunately, jReality bundle only provides jogl native libraries for Linux and MacOS X.
Your system seems to be different.
If you have JOGL installed on your system, please specify its location using options
  --with-jogl and --with-jogl-native for jar files and native libraries respectively;
Alternatively, you may disable using JOGL completely by reconfiguring with option
  --without-jogl
in which case jReality will fall back to a slow software renderer.
---
      $JoglNative="bundled";
   }

   # report status
   return $JoglJars eq "bundled" && $JoglNative eq "bundled" ? "with bundled JOGL" : "with locally installed JOGL";
}
