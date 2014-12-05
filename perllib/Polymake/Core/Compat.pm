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

###############################################################################################
#
#  Compatibility mode: evaluate old-style arguments from the command line
#

my (@request, $expect_subproperty, $subproperties_seen);

package Polymake::User::Compat::Dummy;
my $dummy=bless [ ];
sub AUTOLOAD { $dummy }

package Polymake::User::FilterProps;

sub AUTOLOAD {
   $AUTOLOAD =~ /([^:]+)$/;
   if (@_) {
      die "unknown method or property: $1\n";
   }
   if ($expect_subproperty) {
      $request[-1].=".$1";
      $subproperties_seen=1;
   } else {
      push @request, $1;
   }
   $expect_subproperty=defined(wantarray) && !wantarray;
   $dummy;
}

sub the_rest {
   wantarray ? () : $dummy;
}

sub prepare {
   no strict 'refs';
   while (my ($func, $attr)=each %{$application->EXPORT}) {
      *$func= $attr eq "prop" ? \&$func : \&the_rest;
   }
   @request=();
}

package Polymake::User::Compat;

declare $this;

sub AUTOLOAD {
   my ($what)=$AUTOLOAD =~ /($id_re)$/o;
   if (!defined($this)) {
      die "no current '$this' object\n";
   } elsif (defined (my $prop=$this->type->lookup_property($what))) {
      $this->get($prop);
   } elsif (defined (my $meth=UNIVERSAL::can($this,$what))) {
      $meth->($this,@_);
   } elsif (defined (my $attr=$application->EXPORT->{$what})) {
      die( "object type ", $this->type->full_name, " does not have a ", ($attr eq "meth" ? "method " : "property "), $what, "\n" );
   } else {
      die "unknown function or property: $what\n";
   }
}

sub printable {
   return () unless @_;
   return map { printable($_) } @_ if @_>1;

   my $thing=shift;
   if (is_object($thing)) {
      if (my $proto=UNIVERSAL::can($thing, "typeof")) {
	 # it is one of declared complex types
	 (&$proto)->toString->($thing);

      } elsif (my $ov_array=overload::Method($thing, '@{}')) {
	 # it pretends to be an array
	 @$thing;

      } elsif (my $ov_string=overload::Method($thing, '""') ||
	                     overload::Method($thing, '0+') ||
	                     overload::Method($thing, 'bool')) {
	 # it pretends to be a scalar
	 if (defined (my $value=$ov_string->($thing))) {
	    $value
	 } else {
	    ()
	 }

      } elsif (overload::Method($thing, '*{}') ||
	       UNIVERSAL::can($thing, "TIEHANDLE")) {
	 # it's a wrapped file handle: show nothing
	 ()

      } elsif (my $name=UNIVERSAL::can($thing,'name')) {
	 # at least it has a printable name
	 (defined($thing->name) ? $thing->name.": " : "(anonymous) ").ref($thing);

      } else {
	 # we give up here
	 ref($thing)
      }
   } elsif (is_ARRAY($thing)) {
      # it is a plain array
      @$thing;

   } elsif (ref($thing) eq "GLOB") {
      # it's a file handle: show nothing
      ()

   } elsif (defined($thing)) {
      # it is a plain scalar
      $thing;

   } else {
      # undefined value
      $Polymake::Core::PropertyValue::UNDEF;
   }
}

sub execute {
   my $file=shift;
   local $Scope=new Scope();

   if (-f $file) {
      # old style syntax: FILE PROPERTY ...
      $this=load($file);

      do {
	 package Polymake::User::FilterProps;
	 prepare();
	 my $i=1;
	 foreach (@_) {
	    if (/^ $id_only_re /ox and defined (my $label=$application->prefs->find_label($1))) {
	       if ($i<=$#_) {
		  if ($application->EXPORT->{$1} eq "user") {
		     warn_print( <<"." );
$1 is interpreted as  prefer_now "$1";
but this is probably not exactly the same as you have meant.

The recommended way to enforce using $1 is to call it explicitly:
$1($_[$i])
.
                  }
	          $application->prefer_now($1);
	       } else {
		  warn_print( <<"." );
$1 is a label controlling the choice of functions following it
on the command line.  Being put on the last position it does not
affect anything.
.
               }
	       undef $_;
	    } else {
	       $expect_subproperty=$subproperties_seen=0;
	       my $new_requests=@request;
	       eval $_;
	       if ($@) {
		  $@ =~ s/^Bareword "([^"]+)" not allowed .*$/Unknown method, property, or label: $1/mg unless $DebugLevel;
		  die beautify_error();
	       }
	       if ($subproperties_seen) {
		  foreach my $req (@request[$new_requests..$#request]) {
		     if ((my $expr=$req) =~ s/\./->/g) {
			s/\Q$req\E/$expr/g;
		     }
		  }
	       }
	    }
            ++$i;
	 }
      };

      if ($Core::Scheduler::dry_run) {
	 if (defined (my $chain=$this->get_schedule(@request))) {
	    print $schedule->report, "\n";
	 } else {
	    err_print( "no suitable rules found\n" );
	 }
	 return;
      }

      $this->provide(@request) if @request;

      while (my ($func, $attr)=each %{$application->EXPORT}) {
         no strict 'refs';
         if ($attr eq "prop" || $attr eq "meth") {
	    *$func=\&$func;
         }
      }

      my $pkg=$application->pkg;
      my $exprs=join(",", map { "[ q{$_}.\"\\n\", printable($_) ]" } grep { defined($_) } @_);
      eval <<".";
use strict;
use namespaces '$pkg';
foreach ($exprs) {
   if (\$#\$_>0) {
      enforce_nl(\$_->[-1]);
      print \@\$_, "\n";
   }
}
.
      die beautify_error() if $@;

   } elsif ($file =~ /^ $id_only_re/xo) {
      # slight variation of the old style: FUNCTION FILE ...
      my ($files_seen, $func)=(0);
      foreach my $arg (@_) {
	 if (-f $arg) {
	    $arg=load($arg);
	    ++$files_seen;
	 }
      }
      foreach my $app (list Application) {
	 if ($app->EXPORT->{$file} eq "user") {
	    application($app);
	    $func=namespaces::lookup($app->pkg,$file);
	    last;
	 }
      }
      if ($func) {
	 if ($Core::Scheduler::dry_run) {
	    die "can't call function $file in dry run mode\n";
	 }
	 my $pr=namespaces::lookup($app->pkg, "printable");
	 if (my @pr=$pr->($func->(@_))) {
	    enforce_nl($pr[-1]);
	    print "$file\n", @pr, "\n";
	 }
      } else {
	 die "unknown function or file name: $file\n";
      }
   } else {
      die "file $file does not exist\n";
   }
}


1

# Local Variables:
# c-basic-offset:3
# End:
