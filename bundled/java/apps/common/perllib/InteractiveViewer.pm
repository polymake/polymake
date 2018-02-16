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

require Polymake::Sockets;
require Polymake::Background;

#  Stuff common for the Java-based interactive visualization engines
#  like JavaView or jReality

###########################################################################################
#
#  The central per-process instance, keeping the communication channels
#  to the background java process
#

package Polymake::InteractiveViewer;

use Fcntl ':seek';

use Polymake::Struct (
   '$name',
   '@windows',
   '@new_windows',
   '@pending_windows',
   '%windows_by_id',
   [ '$server_socket' => 'new ServerSocket' ],
   [ '$data_socket' => 'undef' ],
);

my $native_lib_suffix= $^O eq "darwin" ? "jnilib" : "so";

# start the launcher on the java side
sub new {
   $_[0]->instance //= do {
      my $pkg=$_[0];
      my $self=&_new;
      ($self->name)= $pkg =~ /^($id_re)/o;

      my $server_port=ServerSocket::translate_port($self->server_socket->port);
      my $jars_dir= $DeveloperMode ? "$InstallArch/jars" : "$Resources/java/jars";
      my $nativelib_dir= $DeveloperMode ? "$InstallArch/lib/jni" : "$Resources/java/jni";
      my $jre_properties=$self->start_properties;

      my (@jars, @native_libs);
      foreach my $ext ($self->java_extensions) {
         my ($jar, $native_lib);
         if ($ext->is_bundled) {
            $jar="$jars_dir/polymake_".$ext->short_name.".jar";
            $native_lib="polymake_".$ext->short_name;
         } else {
            # @todo
            die "Sorry, loading jars from standalone extensions is not yet implemented\n";
         }
         push @jars, $jar;
         if (-f "$nativelib_dir/lib${native_lib}.${native_lib_suffix}") {
            push @native_libs, $native_lib;
         }
      }
      if (@native_libs) {
         ( $jre_properties->{"java.library.path"}.=":$nativelib_dir" ) =~ s/^://;
      }

      my @java_command=($java,
                        (map { "-D$_=$jre_properties->{$_}" } keys %$jre_properties),
                        $PrivateDir ? ("-Dpolymake.userdir=$PrivateDir") : (),
                        "-cp", join(":", @jars, $self->classpath),
                        "de.tuberlin.polymake.common.SelectorThread",
                        $server_port, map("-nl $_", @native_libs));

      push @java_command, "2>/dev/null" if $DebugLevel<1;

      new Background::Process(@java_command);
      $self->data_socket=new Watcher($self->server_socket->accept, $self);

      if ($Shell->interactive) {
         add AtEnd($self->name, sub { if (defined (my $active=$pkg->instance)) { close($active->data_socket) } },
                   before=>"Background", ignore_multiple=>1);
      }

      $self;
   }
}

sub total_windows {
   my $self=shift;
   @{$self->windows}+@{$self->pending_windows}+@{$self->new_windows}
}

sub append {
   my ($self, $Vis)=@_;
   $self->new_windows->[-1]->contents->append($Vis);
}

sub run {
   my ($self)=@_;
   foreach my $window (splice @{$self->new_windows}) {
      $window->launch($self->data_socket);
      push @{$self->pending_windows}, $window;
   }

   unless ($Shell->interactive || @{$self->windows}) {
      # no interactive shell waiting for input ant a recursive call from a feedback listener:
      # serve the viewer requests until it's finished
      seek $self->data_socket, -1, SEEK_SET;
   }
}

sub find_window {
   my ($self, $pred)=@_;
   grep { $pred->($_) } @{$self->windows};
}

sub shutdown {
   my ($self)=@_;
   undef $self->data_socket;
   undef $self->server_socket;

   foreach my $window (@{$self->windows}) {
      $_->closed for @{$window->feedback_listener};
   }
   undef $self->instance;
}

###########################################################################################
#
#  The helper class, enabling the deadlock-free coexistence with the interactive shell
#  and other background processes.  Does the first processing of incoming feedback messages.
#

package Polymake::InteractiveViewer::Watcher;

use Polymake::Struct (
   [ '@ISA' => 'Pipe::Collaborative' ],
   [ new => '$$' ],
   [ '$viewer' => 'weak( #2 )' ],       # some derived InteractiveViewer instance
   [ '$cur_win' => 'undef' ],
);

sub alone {}

sub in_avail {
   my ($self)=@_;
   my $pos=length($self->rbuffer);
   if ($self->SUPER::in_avail>0) {
      while ($pos < length($self->rbuffer)) {
         pos($self->rbuffer)=$pos;
         if ($self->rbuffer =~ s/\G^x\n//mc) {
            # end of feedback message
            undef $self->cur_win;
         } elsif (defined $self->cur_win) {
            # feedback message not completely consumed yet
            my $cmd=$self->READLINE;
            my $consumed;
            foreach my $listener (@{$self->cur_win->feedback_listener}) {
               $consumed=$listener->feedback($cmd, $self->handle, $self->viewer, $self->cur_win) and last;
            }
            unless ($consumed) {
               chomp $cmd;
               err_print( $self->viewer->name, " sends an unknown command '$cmd'" );
            }
         } elsif ($self->rbuffer =~ s/\G^c\s+(\S+)\n//mc) {
            # window closed
            if (defined (my $wi=delete $self->viewer->windows_by_id->{$1})) {
               my $window=$self->viewer->windows->[$wi];
               $_->closed for @{$window->feedback_listener};
               splice @{$self->viewer->windows}, $wi, 1;
               $_>$wi && --$_ for values %{$self->viewer->windows_by_id};
            } else {
               err_print( $self->viewer->name, " tells about an unknown Window id '$1'" );
            }
         } elsif ($self->rbuffer =~ s/\G^n\s+(\S+)\n//mc) {
            if (defined (my $wi=$self->viewer->windows_by_id->{$1})) {
               # start of a feedback message
               $self->cur_win=$self->viewer->windows->[$wi];
               unless (@{$self->cur_win->feedback_listener}) {
                  err_print( $self->viewer->name, " sends feedback from a non-interactive window $1" );
               }
            } elsif (defined (my $window=shift @{$self->viewer->pending_windows})) {
               # new window opened
               push @{$self->viewer->windows}, $window;
               $window->id=$1;
               $self->viewer->windows_by_id->{$1}=$#{$self->viewer->windows};
            } else {
               err_print( $self->viewer->name, " tells about an unknown Window id '$1'" )
            }
         } else {
            $self->rbuffer =~ s/^(.*)\n//m;
            err_print( $self->viewer->name, " sent an unrecognizable command '", $1 // $self->rbuffer, "'" );
            $self->rbuffer="" if !defined $1;
         }
      }
      1
   }
}

sub CLOSE {
   my ($self)=@_;
   Pipe::CLOSE($self, 1);
   $self->viewer->shutdown;
   1
}

###########################################################################################
#
#  A window for one drawing
#

package Polymake::InteractiveViewer::Window;

use Polymake::Struct (
   [ new => '$' ],
   [ '$contents' => '#1' ],                     # something like JavaView::File
   '$class',                                    # java class controlling the window
   [ '$client_port' => '-1' ],
   '@feedback_listener',
   '$id',                                       # unique ID used in the feedback messages
);

sub launch {
   my ($self, $pipe)=@_;

   if (allow_shmem_exchange()) {
      foreach (@{$self->contents->geometries}) {
         if ($self->detect_dynamic($_->source->Vertices)) {
            $_->name =~ s/^/dynamic:/;
         }
      }
   }

   $_->run($self) for @{$self->feedback_listener};
   my $port=ServerSocket::translate_port($self->client_port);
   if ($DebugLevel>=2) {
      dbg_print( "sending data for ", $self->contents->title, "\n", $self->class, " $port\n", $self->contents->toString );
   }
   print $pipe "n ", $self->class, " $port\n", $self->contents->toString;
}

# temporary measure until Docker on Mac issues with shared memory are solved
sub allow_shmem_exchange {
   if ($ENV{POLYMAKE_HOST_AGENT}) {
      require "$Polymake::InstallTop/resources/host-agent/client.pl";
      state $sys=HostAgent::call("system");
      $sys ne "darwin";
   } else {
      1;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
