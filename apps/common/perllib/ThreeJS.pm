#  Copyright (c) 1997-2020
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

# Interface to ThreeJS.
#
# This file only provides the basic functionality.  Visualization of polymake's various object types
# triggers code implemented in apps/*/rules/threejs.rules.

package ThreeJS;

declare $is_used_in_jupyter=0;

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
   my $resources="$Polymake::Resources/threejs";

   my $random_id = int(rand(100000000000));
   my $explodablemodel = "false";
   my $head=<<"%";
<!--
polymake for $who
$when
$title
-->


<html>
   <head>
      <meta charset=utf-8>
      <title>$title</title>
      <style>
/*
// COMMON_CODE_BLOCK_BEGIN
*/
         html {overflow: scroll;}
         strong{font-size: 18px;}
         canvas { z-index: 8; }
         input[type='radio'] {margin-left:0;}
         input[type='checkbox'] {margin-right:7px; margin-left: 0px; padding-left:0px;}
         .group{padding-bottom: 15px;}
         .settings * {z-index: 11; }
         .settings{z-index: 10; font-family: Arial, Helvetica, sans-serif; margin-left: 30px; visibility: hidden; width: 14em; height: 96%; border: solid 1px silver; padding: 2px; overflow-y: scroll; box-sizing: border-box; background-color: white; position: absolute;}
         .indented{margin-left: 20px; margin-top: 10px; padding-bottom: 0px;} 
         .shownObjectsList{overflow: auto; max-width: 150px; max-height: 150px;}
         .showSettingsButton{visibility: visible; z-index: 12; position: absolute }
         .hideSettingsButton{visibility: hidden; z-index: 12; position: absolute; opacity: 0.5}
         button{margin-left: 0; margin-top: 10px}
         img{cursor: pointer;}
         .suboption{padding-top: 15px;}
         #model$random_id { width: 100%; height: 100%; }
%

      if ($is_used_in_jupyter) {
         $head .= <<"%";
         .threejs_container { width: 100%; height: 75vh;}
         .settings{max-height: 74vh} 
%
      } else {
         $head .= <<"%";
         .threejs_container { width: 100%; height: calc(100vh-16px); }
%
      }

      $head .= <<"%";
         input[type=range] {
           -webkit-appearance: none;
           padding:0; 
           width:90%; 
           margin-left: auto;
           margin-right: auto;
           margin-top: 15px;
           margin-bottom: 15px;
           display: block;	
         }
         input[type=range]:focus {
           outline: none;
         }
         input[type=range]::-webkit-slider-runnable-track {
           height: 4px;
           cursor: pointer;
           animate: 0.2s;
           box-shadow: 0px 0px 0px #000000;
           background: #E3E3E3;
           border-radius: 0px;
           border: 0px solid #000000;
         }
         input[type=range]::-webkit-slider-thumb {
           box-shadow: 1px 1px 2px #B8B8B8;
           border: 1px solid #ABABAB;
           height: 13px;
           width: 25px;
           border-radius: 20px;
           background: #E0E0E0;
           cursor: pointer;
           -webkit-appearance: none;
           margin-top: -5px;
         }
         input[type=range]:focus::-webkit-slider-runnable-track {
           background: #E3E3E3;
         }
         input[type=range]::-moz-range-track {
           height: 4px;
           cursor: pointer;
           animate: 0.2s;
           box-shadow: 0px 0px 0px #000000;
           background: #E3E3E3;
           border-radius: 0px;
           border: 0px solid #000000;
         }
         input[type=range]::-moz-range-thumb {
           box-shadow: 1px 1px 2px #B8B8B8;
           border: 1px solid #ABABAB;
           height: 13px;
           width: 25px;
           border-radius: 20px;
           background: #E0E0E0;
           cursor: pointer;
         }
         input[type=range]::-ms-track {
           height: 4px;
           cursor: pointer;
           animate: 0.2s;
           background: transparent;
           border-color: transparent;
           color: transparent;
         }
         input[type=range]::-ms-fill-lower {
           background: #E3E3E3;
           border: 0px solid #000000;
           border-radius: 0px;
           box-shadow: 0px 0px 0px #000000;
         }
         input[type=range]::-ms-fill-upper {
           background: #E3E3E3;
           border: 0px solid #000000;
           border-radius: 0px;
           box-shadow: 0px 0px 0px #000000;
         }
         input[type=range]::-ms-thumb {
           box-shadow: 1px 1px 2px #B8B8B8;
           border: 1px solid #ABABAB;
           height: 13px;
           width: 25px;
           border-radius: 20px;
           background: #E0E0E0;
           cursor: pointer;
         }
         input[type=range]:focus::-ms-fill-lower {
           background: #E3E3E3;
         }
         input[type=range]:focus::-ms-fill-upper {
           background: #E3E3E3;
         }
/*
// COMMON_CODE_BLOCK_END
*/
		</style>
   </head>
<body>
   <div class='threejs_container'>
		<div id='settings_OUTPUTID' class='settings'>
%
   if (@{$self->geometries}>1) {
      $explodablemodel = "true";
      $head .= <<"%";
			<div class=group id='explode_OUTPUTID'>
				<strong>Explode</strong>
				<input id='explodeRange_OUTPUTID' type='range' min=0 max=6 step=0.01 value=0>
				<div class=indented><input id='explodeCheckbox_OUTPUTID' type='checkbox'>Automatic explosion</div>
				<div class=suboption>Exploding speed</div>
				<input id='explodingSpeedRange_OUTPUTID' type='range' min=0 max=0.5 step=0.001 value=0.05>
			</div>
	
%
   }

   $head .= <<"%";
			<div class=group id='transparency_OUTPUTID' class='transparency'>
				<strong>Transparency</strong>
				<input id='transparencyRange_OUTPUTID' type='range' min=0 max=1 step=0.01 value=0>
			</div>
			
			<div class=group id='rotation_OUTPUTID'>
				<strong>Rotation</strong>
				<div class=indented>
					<div><input type='checkbox' id='changeRotationX_OUTPUTID'> x-axis</div>
					<div><input type='checkbox' id='changeRotationY_OUTPUTID'> y-axis</div>
					<div><input type='checkbox' id='changeRotationZ_OUTPUTID'> z-axis</div>
					<button id='resetButton_OUTPUTID'>Reset</button>
				</div>

				<div class=suboption>Rotation speed</div>
				<input id='rotationSpeedRange_OUTPUTID' type='range' min=0 max=5 step=0.01 value=2>
			</div>


			<div class=group id='display_OUTPUTID'>
				<strong>Display</strong>
				<div class=indented>
					<div id='shownObjectTypesList_OUTPUTID' class='shownObjectsList'></div>
				</div>
				<div class=suboption>Objects</div>
				<div class=indented>
				   <div id='shownObjectsList_OUTPUTID' class='shownObjectsList'></div>
				</div>
			</div>
         
         <div class=group id='camera_OUTPUTID'>
            <strong>Camera</strong>
            <div class=indented>
               <form>
                  <select id="cameraType_OUTPUTID">
                     <option value='perspective' selected> Perspective<br></option>
                     <option value='orthographic' > Orthographic<br></option>
                  </select>
               </form>
            </div>
         </div>

			<div class=group id='svg_OUTPUTID'>
				<strong>SVG</strong>
				<div class=indented>
					<form>
						<input type="radio" name='screenshotMode' value='download' id='download_OUTPUTID' checked> Download<br>
						<input type="radio" name='screenshotMode' value='tab' id='tab_OUTPUTID' > New tab<br>
					</form>
					<button id='takeScreenshot_OUTPUTID'>Screenshot</button>
				</div>
			</div>

		</div>	<!-- end of settings -->
%
   if ($is_used_in_jupyter) {
      $head .= <<"%";
		<img id='hideSettingsButton_OUTPUTID' class='hideSettingsButton' src='/kernelspecs/polymake/close.svg' width=20px">
		<img id='showSettingsButton_OUTPUTID' class='showSettingsButton' src='/kernelspecs/polymake/menu.svg' width=20px">
%
   } else {
         $head .= <<"%";
		<img id='hideSettingsButton_OUTPUTID' class='hideSettingsButton' src='$resources/js/images/close.svg' width=20px">
		<img id='showSettingsButton_OUTPUTID' class='showSettingsButton' src='$resources/js/images/menu.svg' width=20px">
%
   }
   $head .= <<"%";
<div id="model$random_id"></div>
</div>
%

   unless ($is_used_in_jupyter) {
      $head .= "<script src='$resources/js/three.polymake.js'></script>\n";
   }

##############################################################################################
##############################################################################################

   $head .= <<"%";
   <script>
%
   if ($is_used_in_jupyter) {
      $head .= <<"%";
    requirejs.config({
      paths: {
        three: '/kernelspecs/polymake/three',
        TrackballControls: '/kernelspecs/polymake/TrackballControls',
        OrbitControls: '/kernelspecs/polymake/OrbitControls',
        Projector: '/kernelspecs/polymake/Projector',
        SVGRenderer: '/kernelspecs/polymake/SVGRenderer',
        WEBGL: '/kernelspecs/polymake/WebGL',
      },
      shim: {
        'three': { exports: 'THREE'},
        'SVGRenderer': { deps: [ 'three' ], exports: 'THREE.SVGRenderer' },
        'WEBGL': { deps: [ 'three' ], exports: 'THREE.WEBGL' },
        'Projector': { deps: [ 'three' ], exports: 'THREE.Projector' },
        'TrackballControls': { deps: [ 'three' ], exports: 'THREE.TrackballControls' },
        'OrbitControls': { deps: [ 'three' ], exports: 'THREE.OrbitControls' },
      }
    });
    
    require(['three'],function(THREE){
        window.THREE = THREE;
      require(['TrackballControls', 'OrbitControls', 'Projector', 'SVGRenderer', 'WEBGL'],
               function(TrackballControls, OrbitControls, Projector, SVGRenderer, WEBGL) {
    THREE.TrackballControls = TrackballControls;
    THREE.OrbitControls = OrbitControls;
    THREE.Projector = Projector;
    THREE.SVGRenderer = SVGRenderer;
    THREE.WEBGL = WEBGL;
%
   }
   
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
   $head.=<<"%";

// COMMON_CODE_BLOCK_BEGIN

const intervalLength = 25; // for automatic animations
const explodableModel = $explodablemodel; 
const modelContains = { points: false, pointlabels: false, lines: false, edgelabels: false, faces: false, arrowheads: false };
const foldables = [];

var three = document.getElementById("model$random_id");
var scene = new THREE.Scene();
var renderer = new THREE.WebGLRenderer( { antialias: true } );
var svgRenderer = new THREE.SVGRenderer( { antialias: true } );
renderer.setPixelRatio( window.devicePixelRatio );
renderer.setClearColor($bgColor, $bgOp);
svgRenderer.setClearColor($bgColor, $bgOp);
three.appendChild(renderer.domElement);

var frustumSize = 4;
var cameras = [new THREE.PerspectiveCamera($camera_angle, 1, $near, $far), new THREE.OrthographicCamera()];
cameras.forEach(function(cam) {
    cam.position.set($view_point);
    cam.lookAt($view_direction);  
    cam.up.set($view_up);         
});
var controls = [new THREE.TrackballControls(cameras[0], three), new THREE.OrbitControls(cameras[1], three)];
var camera, control;

controls[0].zoomSpeed = 0.2;
controls[0].rotateSpeed = 4;


// class to allow move points together with labels and spheres
var PMPoint = function (x,y,z) {
   this.vector = new THREE.Vector3(x,y,z);
   this.sprite = null;
   this.sphere = null;
}
PMPoint.prototype.addLabel = function(labelsprite) {
   this.sprite = labelsprite;
   this.sprite.position.copy(this.vector);
}
PMPoint.prototype.addSphere = function(spheremesh) {
   this.sphere = spheremesh;
   this.sphere.position.copy(this.vector);
}
PMPoint.prototype.set = function(x,y,z) {
   this.vector.set(x,y,z);
   if (this.sprite) {
      this.sprite.position.copy(this.vector);
   }
   if (this.sphere) {
      this.sphere.position.copy(this.vector);
   }
}
PMPoint.prototype.radius = function() {
   if (this.sphere) {
      return this.sphere.geometry.parameters.radius;
   } else {
      return 0;
   }
};
%

   if ($is_used_in_jupyter) {
      $head.=<<"%";
// select the target node
var target = document.querySelector('#model$random_id');

// create an observer instance
var observer = new MutationObserver(function(mutations) {
   mutations.forEach(function(mutation) {
      if (mutation.removedNodes && mutation.removedNodes.length > 0) {
         cancelAnimationFrame(renderId);
         observer.disconnect();
         console.log("cancelled frame "+renderId);
      }
   });
});

// configuration of the observer:
var config = { childList: true, characterData: true }

// pass in the target node, as well as the observer options
while (target) {
   if (target.className=="output") {
      observer.observe(target, config);
      break;
   }
   target = target.parentNode;
}

%
   }

   $head.=<<"%";
// COMMON_CODE_BLOCK_END

%
   return $head;
}

sub trailer {
   local $/;
   open my $trailer, "$Polymake::Resources/threejs/trailer_string.js" or die $!;
   my $trailerstring = <$trailer>;
   if ($is_used_in_jupyter) {
      # insert function closing at the end
      $trailerstring .= "\n});});\n";
   }
   $trailerstring .= <<"%";
      </script>
   </body>
</html>
%
}

sub toString {
   my ($self)=@_;
   my $trans = $self->transform;
   $self->header($trans) . join("", map { @{$self->geometries}[$_]->toString("obj$_") } (0..@{$self->geometries}-1)) . $self->trailer;
}

##############################################################################################
#
#  Basis class for all graphical objects handled by threejs
#
package ThreeJS::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1->Name' ],
);

sub findCommonProps {
   my ($self, $code_props, $type_props, $props) = @_;
   foreach (@$type_props) {
      if (is_code($self->source->$_)) {
         push @$code_props, $_;
      } else {
         Utils::decor2prop($_, $self->source->$_,$props);
      }
   }
}

sub newMaterial {
   my ($self,$mtype,$type_props,$props,$number)=@_;
   my @code_props = ();
   $self->findCommonProps(\@code_props, $type_props, $props);

   if (@code_props) {
      my @materialstrings;
      if ($number=="use_edge_iter") {
         for (my $e=$self->source->all_edges; $e; ++$e) {
            foreach (@code_props) {
               Utils::decor2prop($_, $self->source->$_->($e),$props);
               push @materialstrings, Utils::materialInstance($mtype,$props);
            }
         }
      } else {
         for (my $i=0; $i<$number; ++$i) {
            foreach (@code_props) {
               Utils::decor2prop($_, $self->source->$_->($i),$props);
               push @materialstrings, Utils::materialInstance($mtype,$props);
            }
         }
      }
      return "[".join(",\n", @materialstrings)."]";
   } else {
      return Utils::materialInstance($mtype,$props);
   }
}

sub writePointMaterial {
   my ($self,$objvar)=@_;
   my $props = { side => "THREE.DoubleSide", transparent => "false"};
   my $mtype = "MeshBasicMaterial";
   my @type_props = ("VertexColor");
   my $string = "   <!-- Vertex style -->\n";
   my $number = @{$self->source->Vertices};
   $string .= "$objvar.userData.pointmaterial = ".$self->newMaterial($mtype, \@type_props, $props, $number).";\n";
   return $string;
}

sub writePoints {
   my ($self,$objvar) = @_;
   my $string = "";
   $string .= "$objvar.userData.points = [];\n";
   my @coords = Utils::pointCoords($self->source->Vertices);
   foreach (@coords) {
      $string .= "$objvar.userData.points.push(new PMPoint($_));\n";
   }
   return $string."\n";
}

sub writePointRadii {
   my ($self,$objvar) = @_;
   my $string = "";
   my $radii = $self->source->VertexThickness // 1;
   my $number = @{$self->source->Vertices};
   if (is_code($radii)) {
      $string .= "$objvar.userData.pointradii = ".Utils::jsArray(map { $radii->($_)/50 } (0..$number)).";\n";
   } else {
      $string .= "$objvar.userData.pointradii = " . $radii/50 . ";\n";   
   }
   return $string;
}

sub writePointLabels {
   my ($self,$objvar) = @_;
   my $string = "";
   my $labels = $self->source->VertexLabels;
   my $number = @{$self->source->Vertices};
   my @labels;
   my $i=-1;
   while ($i < $number-1){
      if (defined(my $label = $labels->(++$i))) {
         $label =~ s/\n/\\n/g;
         push @labels, '"'.$label.'"';
      }
   }
   $string .= "$objvar.userData.pointlabels = ".Utils::jsArray(@labels).";\n";
   return $string;
}

sub writeEdgeMaterial {
   my ($self,$objvar)=@_;
   my $props = { transparent => "false", depthTest => "true"};
   my $mtype = "LineBasicMaterial";
   my @type_props = ("EdgeColor", "EdgeThickness");
   my $string = "   <!-- Edge style -->\n";
   my $number = 0;
   if ($self->source->can("NEdges")) {
      $number = $self->source->NEdges;
   } elsif ($self->source->can("all_edges")) {
      $number = "use_edge_iter";
   }
   $string .= "$objvar.userData.edgematerial = ".$self->newMaterial($mtype, \@type_props, $props, $number).";\n";
   return $string;
}

sub headerString {
   my ($self,$var)=@_;
   my $string = "var $var = new THREE.Object3D();\n";
   $string .= "$var.name = \"".$self->name."\";\n";
   $string .= "$var.userData.explodable = ".$self->source->Explodable.";\n";
}

sub trailerString {
   my ($self,$var)=@_;
   # initializing functions can be found at the beginning of trailer_string.js
   my $string = <<"%";
init_object($var);
scene.add($var);

%
}

sub toString {
   # the logic in here is again used for the init_object method in javascript
   my ($self,$var)=@_;
   my $string = "";
   $var //= "obj";
   $string .= $self->headerString($var);
   $string .= $self->writePoints($var);
   if ($self->source->VertexStyle !~ $Visual::hidden_re) {
      if ($self->source->VertexThickness != 0) {
         $string .= $self->writePointRadii($var);
         $string .= $self->writePointMaterial($var);
      }
      $string .= $self->writePointLabels($var) unless ($self->source->VertexLabels =~ $Visual::hidden_re || $self->source->VertexLabels == "");
   }

   $string .= $self->trailerString($var);
   return $string;
}


##############################################################################################
#
#  Wire model (e.g. a graph)

package ThreeJS::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub writeEdgeIndices {
   my ($self,$objvar)=@_;
   my @allindices;
   for (my $e=$self->source->all_edges; $e; ++$e) {
      push @allindices, @$e;
   }
   return "$objvar.userData.edgeindices = ".Utils::jsArray(@allindices).";\n";
}

sub writeEdgeLabels {
   my ($self,$objvar)=@_;
   my $string = "";
   my $labels = $self->source->EdgeLabels;
   if (is_code($labels)) {
      my @alllabels;
      for (my $e=$self->source->all_edges; $e; ++$e) {
         push @alllabels, $labels->($e);
      }
      $string .= "$objvar.userData.edgelabels = ".Utils::jsArray(@alllabels).";\n";
   } else {
      $string .= "$objvar.userData.edgelabels = ".$labels.";\n";
   }  
   return $string;
}

sub writeArrows {
   my ($self,$objvar)=@_;
   my $string = "";
   my $arrowstyle = $self->source->ArrowStyle;
   if (is_code($arrowstyle)) {
      my @allarrows;
      for (my $e=$self->source->all_edges; $e; ++$e) {
         push @allarrows, $arrowstyle->($e);
      }
      $string .= "$objvar.userData.arrowstyle = ".Utils::jsArray(@allarrows).";\n";
   } else {
      $string .= "$objvar.userData.arrowstyle = ".$arrowstyle.";\n";
   }  
   return $string;
}

sub toString {
   my ($self,$var)=@_;
   my $string = "";
   $var //= "obj";
   $string .= $self->headerString($var);
   $string .= $self->writePoints($var);
   if ($self->source->VertexStyle !~ $Visual::hidden_re) {
      if ($self->source->VertexThickness != 0) {
         $string .= $self->writePointRadii($var);
         $string .= $self->writePointMaterial($var);
      }
      $string .= $self->writePointLabels($var) unless ($self->source->VertexLabels =~ $Visual::hidden_re || $self->source->VertexLabels == "");
   }
   
   $string .= $self->writeEdgeIndices($var);
   if ($self->source->EdgeStyle !~ $Visual::hidden_re) {
      $string .= $self->writeEdgeMaterial($var);
      $string .= $self->writeEdgeLabels($var) unless ($self->source->EdgeLabels =~ $Visual::hidden_re || $self->source->EdgeLabels == "");
      $string .= $self->writeArrows($var) unless ($self->source->ArrowStyle =~ $Visual::hidden_re || $self->source->ArrowStyle == 0);
   }
   
   $string .= $self->trailerString($var);
   return $string;
}

##############################################################################################
#
#  Solid 2-d or 3-d body
#
package ThreeJS::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub writePlanarNetInfo {
   my ($self,$objvar)=@_;
   # extra data for planar net folding
   my $string = "";
   $string .= Utils::convertExtraData($objvar.".userData.axes",$self->source->Axes) if defined $self->source->Axes;
   $string .= Utils::convertExtraData($objvar.".userData.angles",$self->source->DihedralAngles) if defined $self->source->DihedralAngles;
   $string .= Utils::convertExtraData($objvar.".userData.subtrees",$self->source->DependendVertices) if defined $self->source->DependendVertices;
   $string .= Utils::convertExtraData($objvar.".userData.polytoperoot",$self->source->PolytopeRoot) if defined $self->source->PolytopeRoot;
   
   $string .= "$objvar.userData.oldscale = 0;\n";
   $string .= "foldables.push($objvar);\n";
   return $string;
}

sub writeEdgeIndices {
   my ($self,$objvar)=@_;
   my $string = "";
   my $g = graph::graph_from_cycles($self->source->Facets);
   my @allindices;
   foreach my $e (@{$g->EDGES}) {
      push @allindices, @$e;
   }
   $string .= "$objvar.userData.edgeindices = ".Utils::jsArray(@allindices).";\n";
   return $string;
}


sub writeFacetIndices {
   my ($self,$objvar)=@_;
   my $string = "";
   $string .= "$objvar.userData.facets = ".Utils::jsArray(map { Utils::jsArray(@$_) } @{$self->source->Facets}).";\n";
   return $string;
}

sub writeFacetMaterial {
   my ($self,$objvar)=@_;
   my $props = { side => "THREE.DoubleSide", transparent => "true", depthFunc => "THREE.LessDepth", polygonOffset => "true", polygonOffsetFactor => 1, polygonOffsetUnits => 0.5};
   my $mtype = "MeshBasicMaterial";
   my @type_props = ("FacetColor", "FacetTransparency");
   my $text = "   <!-- Facet style -->\n";
   my $number = @{$self->source->Facets};
   $text .= "$objvar.userData.facetmaterial = ".$self->newMaterial($mtype, \@type_props, $props, $number).";\n";
   return $text;
}

sub toString {
   # the logic in here is used for the init_object method in javascript
   my ($self,$var)=@_;
   my $string = "";
   $var //= "obj";
   $string .= $self->headerString($var);
   $string .= $self->writePoints($var);
   if ($self->source->VertexStyle !~ $Visual::hidden_re) {
      if ($self->source->VertexThickness != 0) {
         $string .= $self->writePointRadii($var);
         $string .= $self->writePointMaterial($var);
      }
      $string .= $self->writePointLabels($var) unless ($self->source->VertexLabels =~ $Visual::hidden_re || $self->source->VertexLabels == "");
   }
   
   $string .= $self->writeEdgeIndices($var);
   if ($self->source->EdgeStyle !~ $Visual::hidden_re) {
      $string .= $self->writeEdgeMaterial($var);
   }
   
   $string .= $self->writeFacetIndices($var);
   if ($self->source->FacetStyle !~ $Visual::hidden_re) {
      $string .= $self->writeFacetMaterial($var);
   }

   if ($self->source->isa("Visual::PlanarNet::Polygons")) {
      $string .= $self->writePlanarNetInfo($var);
   }
   $string .= $self->trailerString($var);
   return $string;
}



##############################################################################################

package ThreeJS::Utils;


sub pointCoords {
   my ($data) = @_;
   my @coords = ();
   my $d = is_object($data) ? $data->cols : 3;
   foreach (@{$data}) {
      my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
      $point =~ s/\s+/, /g;
      if ($d == 2) {
         $point.= ", 0";
      }
      if ($d == 1) {
         $point.= ", 0, 0";
      }
      push @coords, $point;
   }
   return @coords;
}

sub rgbToHex {    
	my ($red,$green,$blue) = @_;
   my $hex = sprintf ("0x%2.2X%2.2X%2.2X", $red*255, $green*255, $blue*255);
   return ($hex);
}

sub jsArray {
   "[".join(", ", @_)."]";
}


sub materialInstance {
   my ($material_type, $attr) = @_;
   return "new THREE.$material_type( " . hash2string($attr) . " )";
}

sub decor2prop {
   my ($name, $value, $props) = @_;

   if ($name =~ "Color") {
      $props->{color} = Utils::rgbToHex(@$value);
   }

   if ($name eq "FacetTransparency") {
      my $opacity = defined($value) ? 1-$value : 1;
      $props->{opacity} = $opacity;
   }

   if ($name eq "EdgeThickness") {
      my $width = $value || 1.5;
      $props->{linewidth} = $width;
   }
}

sub hash2string {
   my ($hash) = @_;
   return "{ ".join(", ", map { "$_: $hash->{$_}" } sort keys %$hash)." }"; 
}

sub convertExtraData {
   my ($var,$data) = @_;
   return "$var = [". join(",\n      ", map {ref $_ eq "ARRAY" ? "[".join(",",@$_)."]" : $_ } @$data)."];\n\n";

}

1


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
