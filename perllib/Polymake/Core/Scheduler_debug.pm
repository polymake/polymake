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
use namespaces;

package Polymake::Core::Scheduler::Debug;

use Polymake::Struct (
   [ '$id' => 'undef' ],        # unique identifier
   [ '$children' => '0' ],      # number of variants derived from this chain
);

sub clone {
   my ($self)=@_;
   Struct::make_body(
      (defined($self->id) ? $self->id."." : "").(++$self->children),
      0,
      $self);
}

package Polymake::Core::Scheduler::TentativeRuleChain;

my @textual_code=qw( OK RETRY FAILED INFEASIBLE );

sub describe_rule {
   my ($rule)=@_;
   $rule->header . "   (" . $rule->rgr_node . ")=>[" . join(" ", map { "[@$_]" } @{$rule->prop_vertex_sets}) . "]\n"
}

sub debug_print {
   my ($self, $opname, $heap, $with_graph)=@_;
   local $dbg_prefix="";
   dbg_print( "======= $opname ", $self->debug->id, " ======" );
   if (defined $heap) {
      if (my @weight=tell_weight($self, $heap)) {
         dbg_print( "weight=", @weight);
      }
      if (my ($facet_id, @vertices)=$heap->describe_facet($self)) {
         dbg_print( "facet #$facet_id=[@vertices]" );
      }
   }
   dbg_print( "scheduled:\n", map { describe_rule($_) } @{$self->rules} );
   dbg_print( "ready:\n", map { describe_rule($_) } @{$self->ready} );

   return if !$with_graph || $DebugLevel<=2;

   dbg_print( "graph:\n", map { describe_rule($_),
                                "    supp: ", join(", ", $self->get_active_supplier_nodes($_)), "\n",
                                "    cons: ", join(", ", $self->get_active_consumer_nodes($_)), "\n"
                          } get_active_rules($self) );

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

sub tell_dropped {
   my ($self)=@_;
   local $dbg_prefix="";
   dbg_print( "======= drop ", $self->debug->id, " ======" );
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
