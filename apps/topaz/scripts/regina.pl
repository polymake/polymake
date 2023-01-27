#  Copyright (c) 1997-2023
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


application "topaz";

print "This defines functions for output to regina, see http://regina.sourceforge.net/\n",
      "use like, e.g.,\n",
      '  rga_export_xml(sphere(3),"/tmp/sphere.rga");',"\n",
      "and open that file in regina\n";

sub permutation_as_one_byte(@) {
  my ($a,$b,$c,$d)=@_;
  return ($a + 4*$b + 16*$c + 64*$d);
}

sub rga_export_plain($;$) {
  my ($mfd,$verbose)=@_; # SimplicalComplex, Int
  $verbose=0 if scalar(@_)<2;
  my $text="";

  my $d=$mfd->DIM; # Int
  die "only makes sense for connected pseudo-manifolds of dimension 3" if ($d!=3) or (!$mfd->PSEUDO_MANIFOLD) or (!$mfd->CONNECTED);

  my $facets=$mfd->FACETS; # Array<Set>
  my $n=$facets->size(); # Int
  my $dual_graph=$mfd->DUAL_GRAPH->ADJACENCY; # IncidenceMatrix
  
  for (my $i=0; $i<$n; ++$i) {
    my $this_facet=$facets->[$i]; # Set
    print "$i $this_facet:" if $verbose;
    my @these_vertices=@{$this_facet}; # perl list of Int
    my @all_adjacent_facets=@{$dual_graph->adjacent_nodes($i)}; # perl list of Int
    my %adjacent_facet=(); # "Set" -> Int
    foreach my $f (@all_adjacent_facets) {
      my $ridge = $this_facet * $facets->[$f]; # Set
      $adjacent_facet{"$ridge"} = $f;
    }
    for (my $v=0; $v<=$d; ++$v) { # index of vertex to be skipped
      my @vertices_in_this_ridge = @these_vertices[0..$v-1,$v+1..$d]; # ridge comes in correct order for regina!
      my $this_ridge="{@vertices_in_this_ridge}"; # "Set"
      if (defined(my $f=$adjacent_facet{$this_ridge})) { # Int
	$text .= " $f";
	print " $f $this_ridge" if $verbose;
	my @adjacent_vertices=@{$facets->[$f]};
	my @perm =  map { # gluing permutation to be constructed
	  my $vertex_to_be_mapped=$these_vertices[$_];
	  my $w=0;
	  if ($_ == $v) { # skipped vertex
	    my ($missing_vertex) = @{$facets->[$f]-$this_facet};
	    for (;; ++$w) {
	      last if $adjacent_vertices[$w]==$missing_vertex; # must exist
	    }
	  } else {
	    for (;; ++$w) {
	      last if $adjacent_vertices[$w]==$vertex_to_be_mapped; # must exist
	    }
	  }
	  $w;
	} 0..$d;
	my $pbyte=permutation_as_one_byte(@perm);
	$text .= " $pbyte";
	print " [@perm]-$pbyte" if $verbose;
      } else {
	$text .= " -1 -1";
	print " -1 -1" if $verbose;
      }
    }
    $text .= " \n";
    print "\n" if $verbose;
  }
  return $text;
}

sub rga_export_xml($$;$) {
  my ($mfd,$file,$verbose)=@_; # SimplicalComplex, Int
  $verbose=0 if scalar(@_)<3;

  my @text = split "\n", rga_export_plain($mfd,$verbose);
  my $n = scalar(@text);

  open XML, ">$file";
  print XML << ".";
<?xml version="1.0"?>
<reginadata engine="4.96">
<packet label="polymake output"
	type="3-Manifold Triangulation" typeid="3"
	parent="3-Manifolds">
  <tetrahedra ntet="$n">
.
  foreach (@text) {
    print XML << "."
    <tet>$_</tet>
.
  }
  print XML << ".";
  </tetrahedra>
</packet>
</reginadata>
.
  close XML;
}

1

# Local Variables:
# mode: perl
# c-basic-offset:2
# End:
 

