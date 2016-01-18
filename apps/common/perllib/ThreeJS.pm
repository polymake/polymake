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

# Interface to ThreeJS.
#
# This file only provides the basic functionality.  Visualization of polymake's various object types
# triggers code implemented in apps/*/rules/threejs.rules.


package ThreeJS::File;
use Polymake::Struct (
   [ new => '$' ],
   [ '$title' => '#1' ],
   '@geometries',
   [ '$unnamed' => '0' ],
   [ '$transform' => 'undef' ],
);

sub append {
   my $self = shift;
   push @{$self->geometries}, @_;
   foreach (@_) {
      if (length($_->name)) {
         $self->title //= $_->name;
      } else {
         $_->name="unnamed__" . ++$self->unnamed;
      }
   }
}

sub header {
   my ($self, $trans) = @_;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=$self->title || "unnamed";

	my $source = ${$self->geometries}[0]->source;
   my ($view_point, $view_direction, $view_up, $scale) = $source->transform2view($trans, \%ThreeJS::default::);
	$view_point = join ", ", @$view_point;
	$view_direction = join ", ", @$view_direction;
	$view_up = join ", ", @$view_up;

   my $bgColor = Utils::rgbToHex(@ThreeJS::default::bgColor);
   my $bgOp = $ThreeJS::default::bgOpacity;
   my $camera_angle = $ThreeJS::default::fov;
   my $near = $ThreeJS::default::near_plane;
   my $far = $ThreeJS::default::far_plane;

   return <<"%";
<!--
polymake for $who
$when
$title
-->


<html>
	<head>
		<title>$title</title>
		<style>canvas { width: 100%; height: 100% }</style>
	</head>

<body>

<div id="model"></div>

<script src="js/three.polymake.js"></script>


<script>
	var container = document.getElementById( 'model' );
	var renderer = Detector.webgl? new THREE.WebGLRenderer({antialias: true}): new THREE.CanvasRenderer({antialias: true});

	renderer.setSize(window.innerWidth, window.innerHeight);
	renderer.setClearColor($bgColor, $bgOp);

	container.appendChild(renderer.domElement);


	var scene = new THREE.Scene();
	var camera = new THREE.PerspectiveCamera($camera_angle, window.innerWidth/window.innerHeight, $near, $far);
					
	camera.position.set($view_point);
	camera.lookAt($view_direction);
	camera.up.set($view_up);

	var controls = new THREE.TrackballControls(camera, container);
	
	var all_objects = [];
	var centroids = [];
%
}

sub trailer {
#	my @threejs_ext = grep { $_->URI =~ "http://solros.de/polymake/threejs" } @{User::application("common")->extensions};
#	my $threejs_ext_path = $threejs_ext[0]->dir;

	my $polydir = $Polymake::InstallTop;
	open FILEHANDLE, "$polydir/resources/threejs/trailer_string.html" or die $!;
	do { local $/; <FILEHANDLE> };
}

sub toString {
   my ($self)=@_;
	my $trans = $self->transform;
   $self->header($trans) . join("", map { $_->toString($trans) } @{$self->geometries}) . $self->trailer;
}

##############################################################################################
#
#  Basis class for all graphical objects handled by threejs
#
package ThreeJS::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
);

sub newGeometry {
	my ($self, $var) = @_;
	return <<"%"
	var $var = new THREE.Geometry();
%
}

sub newVertex {
	my ($self, $var, $coords) = @_;
	return <<"%"
	$var.vertices.push(new THREE.Vector3($coords));
%
}

sub newPoint {
	my ($self, $var, $coords, $size, $index) = @_;
	my $r = (is_code($size)) ? $size->($index) : $size;
	return "" unless $r;
	
	my $radius = $r/50;
	my $mat = (is_code($self->source->VertexColor)) ? "materials[$index]" : $var."_material";
	return <<"%"
	var sphere = new THREE.Mesh(new THREE.SphereGeometry($radius), $mat);
	sphere.position.set($coords);
	obj.add(sphere);
%
}

sub newLabel {
	my ($self, $coords, $label) = @_;
	return <<"%"
	makelabel("$label", $coords);
%
}

sub newLine {
	my ($self, $var) = @_;
	my $mat = $var."_material";
	return <<"%"
	obj.add(new THREE.Line($var, $mat));
	
%
}


sub newMaterial {
	my ($self, $var, $type) = @_;
	my $matvar = $var."_material";

	my $material;
	my @code_props = ();
	my @type_props;
	my $number = 0;

	my $text = "	<!-- $type style -->\n";

	if ($type eq "Vertex") {
		$material = "MeshBasicMaterial";
		@type_props = ("VertexColor");
		$number = @{$self->source->Vertices};
	} elsif ($type eq "Facet") {
		$material = "MeshBasicMaterial";		
		@type_props = ("FacetColor", "FacetTransparency");
		$number = @{$self->source->Facets};
	} elsif ($type eq "Edge") {
		$material = "LineBasicMaterial";	
		@type_props = ("EdgeColor", "EdgeThickness");
		
		
		if ($self->source->can("NEdges")) {
			$number = $self->source->NEdges;
		} elsif ($self->source->can("n_edges")) {
			$number = $self->source->n_edges;
		}
	} else {
		# should not happen
	}
	my $common_string = $self->find_common_string(\@code_props, \@type_props);		

	if (@code_props) {
#		die "Edge decorations must be constants when using three.js." if $type eq "Edge";
		if ($type eq "Edge") {
			return $text . $self->edgeMaterial($matvar, $material, $common_string, \@code_props);
		}
		return $text . $self->codeMaterial($matvar, $material, $common_string, \@code_props, $number);
	} else {
		return $text . Utils::constantMaterial($matvar, $material, "{".$common_string."}");
	}
}



sub header {
	return <<"%"
	var obj = new THREE.Object3D();
	
%
}

sub trailer {
	return <<"%"
	scene.add(obj);
	all_objects.push(obj);

%
}


sub pointsToString {
    my ($self, $var)=@_;
    
   my $labels = $self->source->VertexLabels;

   my $text = "";

	if ($self->source->VertexStyle !~ $Visual::hidden_re){
		my @coords = Utils::pointCoords($self);

		my $thickness = $self->source->VertexThickness || 1;
		

		$text .= $self->newMaterial("points", "Vertex");

		$text .= "\n	<!-- POINTS -->\n";
		
		my $i = 0;
		foreach (@coords){
			$text .= $self->newPoint($var, $_, $thickness, $i++);
		}
		if ($labels !~	$Visual::hidden_re && $labels != "") {
			my $i=-1;
			foreach (@coords){
				if (defined(my $label = $labels->(++$i))) {
					$text .= $self->newLabel($_, $label);
				}
			}
		}
		
		$text .= "\n";
	}
	
	return $text;
}

sub verticesToString {
	my ($self, $var) = @_; 

	my $text = "";
	
    my @coords = Utils::pointCoords($self);
    $text .= "\n   <!-- VERTICES -->\n";
    foreach (@coords) {
        $text .= $self->newVertex($var, $_);
   }

	$text .= <<"%";
	
	centroids.push(computeCentroid($var));

%

	return $text;
}


sub toString {
    my ($self, $trans)=@_;
    $self->header . $self->pointsToString("points") . $self->trailer;
}



sub find_common_string {
	my ($self, $code_props, $type_props) = @_;
	
	my $common_string = "";
	foreach (@$type_props) {
		if (is_code($self->source->$_)) {
			push @$code_props, $_;
		} else {
			$common_string .= Utils::writeDecor($_, $self->source->$_);
		}
	}
	return $common_string;
}


sub codeMaterial {
	my ($self, $matvar, $material_type, $common_string, $code_props, $number) = @_;
	
	my $text = "	var materials = [\n";

	for (my $i=0; $i<$number; ++$i) {
		$text .= $self->oneCodeMaterial($i, $material_type, $common_string, $code_props);
	}

	$text .= "	];\n";

	return $text .= Utils::constantMaterial($matvar, "MeshFaceMaterial", "materials");
}

sub oneCodeMaterial {
	my ($self, $index, $material_type, $common_string, $code_props) = @_;

	my $text = "		new THREE.".$material_type."({ ".$common_string;
	
	foreach (@$code_props) {
		$text .= Utils::writeDecor($_, $self->source->$_->($index));
	}
	
	$text .= "}),\n";
	
	return $text;
}

sub edgeMaterial {
	my ($self, $matvar, $material_type, $common_string, $code_props) = @_;

	my $text = "		var $matvar = new THREE.".$material_type."({ " . $common_string;
	
	foreach (@$code_props) {
		$text .= Utils::writeDecor($_, $self->source->$_->(0));
	}
	$text .= "});\n";
	
	return $text;
}

##############################################################################################
#
#  Wire model (e.g. a graph)

package ThreeJS::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub newEdge {
	my ($self, $var, $a, $b) = @_;
	
	return $self->newGeometry($var) . $self->newVertex($var, $a) . $self->newVertex($var, $b) . $self->newLine($var);
}


sub linesToString {
    my ($self, $var)=@_;

#	  TODO: arrows
#    my $arrows=$self->source->ArrowStyle;

 #   if($arrows==1) {
 #       TODO
 #   } elsif($arrows==-1) {
 #       TODO
 #   }

    my $text ="";

	if ($self->source->EdgeStyle !~ $Visual::hidden_re) {
		$text .= $self->newMaterial($var, "Edge");

		$text .= "\n	<!-- EDGES -->\n";

		my @coords = Utils::pointCoords($self);

		for (my $e=$self->source->all_edges; $e; ++$e) {
			$text .= $self->newEdge($var, $coords[$e->[0]], $coords[$e->[1]]);
		}
	}
	
	return $text;
}

sub toString {
    my ($self, $trans)=@_;
   $self->header . $self->pointsToString("points") . $self->linesToString("line") . $self->trailer;
}


##############################################################################################
#
#  Solid 2-d or 3-d body
#
package ThreeJS::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);


sub newFace {
	my ($self, $var, $indices, $facet, $facet_color) = @_;
	my $m_index = 0;
	if (is_code($facet_color) || is_code($self->source->FacetTransparency)) {
		$m_index = $facet;
	}
	return <<"%"
	$var.faces.push(new THREE.Face3($indices, undefined, undefined, $m_index));
%
}


sub facesToString {
    my ($self, $trans, $var)=@_;

    my $text = "";

	### FACETS
	my $facets = new Array<Array<Int>>($self->source->Facets);	

	if ($self->source->FacetStyle !~ $Visual::hidden_re){
		$text .= $self->newGeometry($var);
		$text .= $self->verticesToString($var);

		$text .= $self->newMaterial($var, "Facet");

    
    	# draw facets
		$text .= "\n   <!-- FACETS --> \n";  
		my $facet_color = $self->source->FacetColor;
		for (my $facet = 0; $facet<@$facets; ++$facet) {
			# triangulate the facet
			for (my $triangle = 0; $triangle<@{$facets->[$facet]}-2; ++$triangle) {
				my @vs = @{$facets->[$facet]}[0, $triangle+1, $triangle+2];
				$text .= $self->newFace($var, join(", ", @vs), $facet, $facet_color);
			}
			$text.="\n";
		}

		my $mat = $var."_material";
		$text .= <<"%"
	
	$var.computeFaceNormals();
	$var.computeVertexNormals();
	
	var object = new THREE.Mesh($var, $mat);
	obj.add(object);
	
%
	}
	



	## EDGES
	if ($self->source->EdgeStyle !~ $Visual::hidden_re){
		$var = "line";
		
		$text .= $self->newMaterial($var, "Edge");		
    
    	# draw edges
		$text .= "\n   <!-- EDGES --> \n";  
		my @coords = Utils::pointCoords($self);

		for (my $facet = 0; $facet<@$facets; ++$facet) {
			$text .= $self->newGeometry($var);
			
			foreach (@{$facets->[$facet]}) {
				$text .= $self->newVertex($var, $coords[$_]);
			}
			# first vertex again
			$text .= $self->newVertex($var, $coords[$facets->[$facet]->[0]]);
			$text .= $self->newLine($var);
		}
	}
	

	return $text;
}

sub toString {
	my ($self, $transform)=@_;
	return $self->header . $self->pointsToString("points") . $self->facesToString($transform, "faces") . $self->trailer;
}


##############################################################################################

package ThreeJS::Utils;

sub rgbToHex {    
	my $red=shift;
	my $green=shift;
	my $blue=shift;
	my $hex = sprintf ("0x%2.2X%2.2X%2.2X", $red*255, $green*255, $blue*255);
	return ($hex);
}

sub pointCoords {
	my ($self) = @_;
	my @coords = ();
   my $d = is_object($self->source->Vertices) ? $self->source->Vertices->cols : 3;
	foreach (@{$self->source->Vertices}) {
#		print $_."		";
		my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
		$point =~ s/\s+/, /g;
		if ($d == 2) {
			$point.= ", 0";
		}
		if ($d == 1) {
			$point.= ", 0, 0";
		}
#		print $point."\n";
		push @coords, $point;
	}
	return @coords;
}


sub writeDecor {
	my ($name, $value) = @_;
	
	if ($name =~ "Color") {
		return "color: " . Utils::rgbToHex(@$value) . ", ";
	}
	
	if ($name eq "FacetTransparency") {
		my $opacity = defined($value) ? 1-$value : 1;
		if ($opacity == 1) {
			return "";
		}
		return "transparent: true, opacity: $opacity, ";
	}
	
	if ($name eq "EdgeThickness") {
		my $width = $value || 1;
		return "linewidth: $width, ";
	}
}


sub constantMaterial {
	my ($matvar, $material_type, $material_string) = @_;

	return << "%";	
	var $matvar = new THREE.$material_type ( $material_string );
	$matvar.side = THREE.DoubleSide;
	
%
}




1

