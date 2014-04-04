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

package Polymake::Core::Scheduler::TentativeRuleChain;

my @textual_code=qw( OK RETRY FAILED INFEASIBLE );

sub dump {
   my $self=shift;
   local $dbg_prefix="";
   dbg_print( "weight=", $self->weight );
   dbg_print( map { $_->header, "\n" } @{$self->rules} );
   dbg_print( "ready:\n", map { $_->header, "\n" } @{$self->ready} );
   if (keys %{$self->pending_perms}) {
      dbg_print( "pending:\n", map { map { $_->header, "\n" } keys %$_ } values %{$self->pending_perms} );
   }

   return if $DebugLevel<=2;

   if (!is_hash($self->supplier)) {
      dbg_print( "invalid field 'supplier': ", $self->supplier );
      return;
   }

   if (!is_hash($self->consumer)) {
      dbg_print( "invalid field 'consumer': ", $self->consumer );
      return;
   }

   while (my ($rule, $supp_list)=each %{$self->supplier}) {
      if (is_object($rule)) {
         if (!is_ARRAY($supp_list)) {
            if ($supp_list ne '0') {
               dbg_print( "invalid supplier list of ", $rule->header, ": $supp_list" );
            }
            next;
         }
         dbg_print( "supplier of ", $rule->header, ":\n" );
         foreach my $supp_group (@$supp_list) {
            if (!is_hash($supp_group)) {
               dbg_print( "invalid element in supplier list: $supp_group\n" );
               next;
            }
            dbg_print( (map { ("   ", $_->header, "\n") } keys %$supp_group),
                       "---" );
         }
      } else {
         if (ref($supp_list) ne "ARRAY") {
            dbg_print( "invalid multiple chooser mask for ", @$rule ? $rule->[0]->header : " EMPTY chooser bag" );
            next;
         }
         dbg_print( "multiple chooser bag\n",
                    (map { ("   ", $_->header, "\n") } @$rule),
                    "   common mask: (", join(",", @$supp_list), ")\n---" );
      }
   }

   while (my ($rule, $cons)=each %{$self->consumer}) {
      if (!is_hash($cons)) {
	 dbg_print( "invalid consumer list of ", $rule->header, ": $cons" );
	 next;
      }
      dbg_print( "consumer of ", $rule->header, ":\n",
		 (map { ("   ", $_->header, "\n") } keys %$cons), "---" );
   }

   return unless instanceof InitRuleChain($self);

   if (!is_hash($self->run)) {
      dbg_print( "invalid field 'run': ", $self->run );
   } else {
      dbg_print( "exec codes:\n" );
      while (my ($rule, $code)=each %{$self->run}) {
	 dbg_print( $rule->header, " -> ",
		    is_object($code) ? $code : $textual_code[$code] );
      }
   }
}

sub dump_list($) {
   my ($chainlist)=@_;
   foreach my $chain (@$chainlist) {
      dbg_print( $chain->debug->id );
      $chain->dump;
      dbg_print( "\n" );
   }
}

####################################################################################

package Polymake::Core::Scheduler::VerboseHeap;
@ISA=( 'Polymake::Core::Scheduler::Heap' );

sub reset {
   my ($self, $chain)=@_;
   dbg_print( "====== reset ", $chain->debug->id, " ======" );
   $chain->dump;
   &Heap::reset;
}

sub push {
   my ($self, $chain)=@_;
   dbg_print( "======= push ", $chain->debug->id, " ======" );
   $chain->dump;
   @{$chain->ready} or die "attempt to push a dead variant to heap!\n";
   &Heap::push;
}

sub pop {
   if (defined (my $chain=&Heap::pop)) {
      dbg_print( "======= pop ", $chain->debug->id, " =======" );
      $chain->dump;
      defined($chain->final) or die "dead variant popped off from heap!\n";
      $chain;
   } else {
      dbg_print( "======= Heap is empty =======" );
      undef;
   }
}

sub find {
   my ($self, $id)=@_;
   foreach my $chain (@$self) {
      return $chain if $chain->debug->id eq $id;
   }
}

1

# Local Variables:
# c-basic-offset:3
# End:
