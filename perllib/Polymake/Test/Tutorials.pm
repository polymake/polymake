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
use feature 'state';

use JSON;

package Polymake::Test::Tutorials;

use Polymake::Struct (
   [ '@ISA' => 'Group' ],
   [ 'new' => '$$$' ],
   [ '$name' => '"tutorials.Tutorials"' ],
   [ '$env' => '#1' ],
   [ '$application' => 'undef' ],
   [ '$pattern' => '#2' ],
   [ '$extension' => 'undef' ],
   [ '$base_dir' => '#3' ],
);

sub new {
   my $self=&_new;
   # pattern might be a path or a wildcard
   push @{$self->subgroups},
      map { new Tutorial($_,$self) }
         ( -f $self->pattern ?
            ($self->pattern) :
            grep { -f } glob $self->base_dir."/".$self->pattern.".ipynb");
   $self;
}

sub package_name {
   "tutorials"
}

sub run_context { undef }

package Polymake::Test::Tutorial;

#one tutorial
use Polymake::Struct (
   [ '@ISA' => 'Subgroup' ],
   [ 'new' => '$$' ],
   [ '$source_file' => '#1' ],
   [ '$name' => 'undef' ],
   [ '$group' => 'weak(#2)' ],
   [ '$temp_dir' => 'undef' ],
);

sub new {
   my $self=&_new;
   my ($name) = $self->source_file =~ m/\/([^\/]*)\.ipynb$/g;
   $self->name = $name;
   $self;
}

sub create_testcases {
   my ($self)=@_;
   state $testpkg="TutorialTestPkg000000";
   ++$testpkg;

   my $file_name = $self->source_file;
   my $json = '';
   open my $in, "<:encoding(utf8)", $file_name
      or die("Error opening $file_name: $!");
   while (my $line = <$in>) {
      $json .= $line;
   }
   close $in or die $!;

   my $p = JSON->new->decode($json);
   my $name = $self->name;
   my $app = $User::default_application;
   my $body= <<"---";
$Polymake::Core::warn_options; 
package Polymake::$testpkg;
use application '$app';  declare +auto;
include "common::jupyter.rules";
# testing $name
---

   $self->group->env->start_timers;
   my $cnt=0;
   foreach (@{${$p}{"cells"}}) {
      if (${$_}{"cell_type"} eq "code") { # iterate all code cells
	 # each cell might introduce independent local effects
	 # $Scope is localized in Group::run, no need to localize it here again
	 $body .= "\$Scope=new Scope();\n" if $cnt++;

         foreach (@{${$_}{"source"}}) {
	    s{^ \s* application \s* ($anon_quoted_re) \s*;\s* $}
	     {use application $1;\n}xmg;

            $body .= enforce_nl($_);  # add a trailing newline after the command if not present (for ease of debugging)
         }
      }
   }
   $body .= <<"---";
delete \$Polymake::{"$testpkg\::"};
---

   # always create a tempdir
   $self->temp_dir //= new Polymake::Tempdir;

   # copy input files if demo/files/<basename> exists
   # should be opened with something like
   # load("files/<basename>/someexample.poly")
   if (-d $self->group->base_dir."/files/".$self->name) {
      File::Path::make_path($self->temp_dir."/files/");
      system("cp -RLp '".$self->group->base_dir."/files/".$self->name."/' '".$self->temp_dir."/files/'");
   }
   new TutorialCase("body", $body, $self->source_file);
}

package Polymake::Test::TutorialCase;

use Polymake::Struct (
   [ '@ISA' => 'Output' ],
   [ 'new' => '$$$' ],
   [ '$name' => '#1' ],
   [ '$body' => '#2' ],
   [ '$source_file' => '#3' ],
);

sub new {
   my $self=&Case::new;
   local $disable_viewers = 1;
   my $tdir = new TempChangeDir($self->subgroup->temp_dir);
   # files created in tutorials via save(), save_data() etc. must be automatically destroyed
   # script() must be exempt from this transformation because it refers to existing scripts
   local ref *replace_special_paths = sub {
      if ((caller(1))[3] ne "Polymake::User::script") {
	 foreach (@_) {
	    s{^~(?=/|$)}{ $self->subgroup->temp_dir }e
	      or
	    s{^(?=[^/])}{ $self->subgroup->temp_dir."/" }e;
	 }
      }
   };
   before_run($self);
   eval $self->body;
   if ($@) {
      $self->gotten_error=neutralized_ERROR();
      $@="";
   }
   after_run($self);
   close $self->handle;
   $self;
}

sub execute {
   my ($self)=@_;
   if (length($self->gotten_error)) {
      $self->fail_log="expected: regular return\n".
                      "     got: EXCEPTION: ".$self->gotten_error;
      return 0;
   }

   1
}

1
