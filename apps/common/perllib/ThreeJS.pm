#  Copyright (c) 1997-2019
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

   my $random_id = int(rand(100000000000));

   my $width_height_start = "";
   my $width = "document.body.clientWidth - 20";
   my $height = "document.body.clientHeight - 20";


   if ($is_used_in_jupyter) {
      $width_height_start = <<"%";
         var box = document.getElementsByClassName( 'output_subarea' )[0];
         var notebook = document.getElementById( 'notebook_panel' );
%
      $width = "box.clientWidth - 25";
      $height = "notebook.clientHeight * 0.8";
   }

   my $head=<<"%";
<!--
polymake for $who
$when
$title
-->


<html>
   <head>
      <title>$title</title>
      <style>
/*
// COMMON_CODE_BLOCK_BEGIN
*/
         html{overflow: scroll;}
         body { font-family: Arial, Helvetica, sans-serif}
         strong{font-size: 18px;}
         canvas { z-index: 8; }
         input[type='range'] {}
         input[type='radio'] {margin-left:0;}
         input[type='checkbox'] {margin-right:7px; margin-left: 0px; padding-left:0px;}
         .group{padding-bottom: 40px;}
         .settings * {z-index: 11; }
         .settings{z-index: 10; margin-left: 30px; display: none; width: 14em; height: 90%; border: solid 1px silver; padding: 2px; overflow-y: scroll; background-color: white }
         .indented{margin-left: 20px; margin-top: 15px; padding-bottom: 0px;} 
         .shownObjectsList{overflow: auto; max-width: 150px; max-height: 150px;}
         .showSettingsButton{display: block; z-index: 12; position: absolute }
         .hideSettingsButton{display: none; z-index: 12; position: absolute; opacity: 0.5}
         .resetButton{margin-top: 20px;}
         button{margin-left: 0;}
         img{cursor: pointer;}
         .suboption{padding-top: 30px;}
         .transparency{display: none;}
         .labelsCheckbox{margin-top: 10px;}


         input[type=range] {
           -webkit-appearance: none;
           padding:0; 
           width:90%; 
           margin-left: auto;
           margin-right: auto;
           margin-top: 20px;
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

		<div id='settings_OUTPUTID' class='settings'>
%
   if (@{$self->geometries}>1) {
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

   if ($source->isa("Visual::PlanarNet::Polygons")) {
      $head .= <<"%";
			<div class=group id='fold_OUTPUTID'>
				<strong>Fold</strong>
				<input id='foldRange_OUTPUTID' type='range' min=0 max=1 step=0.001 value=0>
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
					<button id='resetButton_OUTPUTID' class='resetButton' >Reset</button>
				</div>

				<div class=suboption>Rotation speed</div>
				<input id='rotationSpeedRange_OUTPUTID' type='range' min=0 max=5 step=0.01 value=2>

			</div>


			<div class=group id='display_OUTPUTID'>
				<strong>Display</strong>
				<div class=indented>
					<div id='shownObjectsList_OUTPUTID' class='shownObjectsList'></div>
					<div class='labelsCheckbox'><input type='checkbox' id='labelsCheckboxInput_OUTPUTID' checked>Labels</div>
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
                if($is_used_in_jupyter){
                $head .= <<"%";
		<img id='hideSettingsButton_OUTPUTID' style="display: none" class='hideSettingsButton' src='/kernelspecs/polymake/close.svg' width=20px">
		<img id='showSettingsButton_OUTPUTID' class='showSettingsButton' src='/kernelspecs/polymake/menu.svg' width=20px">
%
              }
              else
              {
              $head .= <<"%";
		<img id='hideSettingsButton_OUTPUTID' class='hideSettingsButton' src='$resources/js/images/close.svg' width=20px">
		<img id='showSettingsButton_OUTPUTID' class='showSettingsButton' src='$resources/js/images/menu.svg' width=20px">
%
              }
   $head .= <<"%";
<div id="model$random_id"></div>

%

   unless ($is_used_in_jupyter) {
      $head .= "<script src='$resources/js/three.polymake.js'></script>\n";
   }

   $head .= <<"%";
<script>
%

   if ($is_used_in_jupyter) {
      $head .= <<"%";
requirejs.config({
  paths: {
    three: '/kernelspecs/polymake/three',
    Detector: '/kernelspecs/polymake/Detector',
    SVGRenderer: '/kernelspecs/polymake/SVGRenderer',
    CanvasRenderer: '/kernelspecs/polymake/CanvasRenderer',
    Projector: '/kernelspecs/polymake/Projector',
    TrackballControls: '/kernelspecs/polymake/TrackballControls'
  },
  shim: {
    'three':
    {
      exports: 'THREE'
    },
    'Detector':
    {
      deps: [ 'three' ],
      exports: 'Detector'
    },
    'SVGRenderer':
    {
      deps: [ 'three' ],
      exports: 'THREE.SVGRenderer'
    },
    'CanvasRenderer':
    {
      deps: [ 'three' ],
      exports: 'THREE.CanvasRenderer'
    },
    'Projector':
    {
      deps: [ 'three' ],
      exports: 'THREE.Projector'
    },
    'TrackballControls':
    {
      deps: [ 'three' ],
      exports: 'THREE.TrackballControls'
    }
  }
});
require(['three'],function(THREE){
    window.THREE = THREE;
  require(['Detector','SVGRenderer','CanvasRenderer','Projector','TrackballControls'],function(Detector,SVGRenderer,CanvasRenderer,Projector,TrackballControls){
      THREE.SVGRenderer = SVGRenderer;
      THREE.CanvasRenderer = CanvasRenderer;
      THREE.Projector = Projector;
      THREE.TrackballControls = TrackballControls;

%
   }


   $head .= <<"%";
// COMMON_CODE_BLOCK_BEGIN
	var foldable = false;
%
   if ($source->isa("Visual::PlanarNet::Polygons")){
      $head.=<<"%";

		foldable = true;

// rotate point p around axis defined by points p1 and p2 by given angle
function rotate(p, p1, p2, angle ){   
   angle = -angle;
   var x = p.x, y = p.y, z = p.z, 
   a = p1.x, b = p1.y, c = p1.z, 
   u = p2.x-p1.x, v = p2.y-p1.y, w = p2.z-p1.z;
   var result = [];
   var L = u*u + v*v + w*w;
   var sqrt = Math.sqrt;
   var cos = Math.cos;
   var sin = Math.sin;

   result[0] = ((a*(v*v+w*w)-u*(b*v+c*w-u*x-v*y-w*z))*(1-cos(angle))+L*x*cos(angle)+sqrt(L)*(-c*v+b*w-w*y+v*z)*sin(angle))/L;
   result[1] = ((b*(u*u+w*w)-v*(a*u+c*w-u*x-v*y-w*z))*(1-cos(angle))+L*y*cos(angle)+sqrt(L)*(c*u-a*w+w*x-u*z)*sin(angle))/L;
   result[2] = ((c*(u*u+v*v)-w*(a*u+b*v-u*x-v*y-w*z))*(1-cos(angle))+L*z*cos(angle)+sqrt(L)*(-b*u+a*v-v*x+u*y)*sin(angle))/L;

   return result;
}

var oldScale = 0;
function fold(event){
   if (typeof(event) == 'number'){
      var x = event;
   }
   else var x = Number(event.currentTarget.value);

   var scale = x - oldScale;

   for (var j=0; j<axes.length; j++){
      rotateVertices(j, scale);
   }
   update();
   oldScale += scale;
   moveToBaryCenter();
}

function moveToBaryCenter(){
   controls.target = barycenter();
}

function barycenter(){
   var center = new THREE.Vector3(0,0,0);
   for (var i=0; i<allpoints.length; i++){
      center.add(allpoints[i].vector);
   }
   center.divideScalar(allpoints.length);
   return center;
}

function rotateVertices(edge, scale){
   if (edge < axes.length){
      for (var j=0; j<subtrees[edge].length; j++){
         var rotP = rotate(allpoints[subtrees[edge][j]].vector, allpoints[axes[edge][0]].vector,allpoints[axes[edge][1]].vector , scale * (Math.PI - angles[edge]));
         allpoints[subtrees[edge][j]].set(rotP[0],rotP[1],rotP[2]);
      }
   }
}


function update(){
   for (index = 0; index < obj.children.length; ++index) { 
      if (obj.children[index] instanceof THREE.Line || obj.children[index] instanceof THREE.Mesh) {
            obj.children[index].geometry.verticesNeedUpdate=true; 
      } 
   }
}

%
   }

   $head.=<<"%";
   var container = document.getElementById( 'model$random_id' );
   var renderer = Detector.webgl? new THREE.WebGLRenderer({antialias: true}): new THREE.CanvasRenderer({antialias: true});
	var svgRenderer = new THREE.SVGRenderer({antialias: true});
   $width_height_start
   var width = $width;
   var height = $height;
   renderer.setSize(width, height);
   svgRenderer.setSize(width, height);
   renderer.setClearColor($bgColor, $bgOp);
   svgRenderer.setClearColor($bgColor, $bgOp);

   container.appendChild(renderer.domElement);

   var scene = new THREE.Scene();
   var camera = new THREE.PerspectiveCamera($camera_angle, width/height, $near, $far);

   var renderid;

   camera.position.set($view_point);
   camera.lookAt($view_direction);
   camera.up.set($view_up);

   // class to allow move points together with labels and spheres
   var PMPoint = function (x,y,z) {
      this.vector = new THREE.Vector3(x,y,z);
      this.sprite = null;
      this.sphere = null;
   }
   PMPoint.prototype.makelabel = function(label) {
      this.sprite = textSprite( label );
      this.sprite.position.copy(this.vector);
   }
   PMPoint.prototype.makesphere = function(radius,material) {
      this.sphere = new THREE.Mesh(new THREE.SphereGeometry(radius), material);
      this.sphere.position.copy(this.vector);
   }

   PMPoint.prototype.setX = function(x) {
      this.vector.setX(x);
      if (this.sprite) {
         this.sprite.position.setX(x);
      }
      if (this.sphere) {
         this.sphere.position.setX(x);
      }
   };
   PMPoint.prototype.setY = function(y) {
      this.vector.setY(y);
      if (this.sprite) {
         this.sprite.position.setY(y);
      }
      if (this.sphere) {
         this.sphere.position.setY(y);
      }
   };
   PMPoint.prototype.setZ = function(z) {
      this.vector.setZ(z);
      if (this.sprite) {
         this.sprite.position.setZ(z);
      }
      if (this.sphere) {
         this.sphere.position.setZ(z);
      }
   };
   PMPoint.prototype.set = function(x,y,z) {
      this.vector.set(x,y,z);
      if (this.sprite) {
         this.sprite.position.set(x,y,z);
      }
      if (this.sphere) {
         this.sphere.position.set(x,y,z);
      }
   };
   PMPoint.prototype.add = function(o) {
      if (this.sprite) {
         o.add(this.sprite);
      }
      if (this.sphere) {
         o.add(this.sphere);
      }
   };


   var controls = new THREE.TrackballControls(camera, container);
	controls.zoomSpeed = 0.2;
	controls.rotateSpeed = 4;

   var all_objects = [];
   var centroids = [];
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
   $head .= '   var objectnames = ["'.join('","',map { $_->name } @{$self->geometries})."\"];\n";
   if ($source->isa("Visual::PlanarNet::Polygons")) {
      # extra data for planar net folding
      $head .= Utils::convertExtraData("axes",$source->Axes) if defined $source->Axes;
      $head .= Utils::convertExtraData("angles",$source->DihedralAngles) if defined $source->DihedralAngles;
      $head .= Utils::convertExtraData("subtrees",$source->DependendVertices) if defined $source->DependendVertices;
      $head .= Utils::convertExtraData("polytoperoot",$source->PolytopeRoot) if defined $source->PolytopeRoot;
   }
   return $head;
}

sub trailer {
   local $/;
   open my $trailer, "$Polymake::Resources/threejs/trailer_string.html" or die $!;
   my $trailerstring = <$trailer>;
   if ($is_used_in_jupyter) {
      # insert function closing at the end
      $trailerstring =~ s#(?=// COMMON_CODE_BLOCK_END)#});});\n#m;
   }
   $trailerstring;
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

sub newCoord {
   my ($self, $coords) = @_;

   return <<"%"
   allpoints.push(new PMPoint($coords));
%
}

sub newVertex {
   my ($self, $var, $index) = @_;
   return <<"%"
   $var.vertices.push(allpoints[$index].vector);
%
}

sub newPoint {
   my ($self, $var, $size, $index) = @_;
   my $r = (is_code($size)) ? $size->($index) : $size;
   return "" unless $r;

   my $radius = $r/50;
   my $mat = (is_code($self->source->VertexColor)) ? "materials[$index]" : $var."_material";
   return <<"%"
   allpoints[$index].makesphere($radius,$mat);
%
}

sub newLabel {
   my ($self, $index, $label) = @_;
   return <<"%"
   allpoints[$index].makelabel("$label");
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

   my $text = "   <!-- $type style -->\n";

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
#     die "Edge decorations must be constants when using three.js." if $type eq "Edge";
      if ($type eq "Edge") {
         return $text . $self->edgeMaterial($matvar, $material, $common_string, \@code_props);
      }
      return $text . $self->codeMaterial($matvar, $material, $common_string, \@code_props, $number);
   } else {
      $text .= Utils::constantMaterial($matvar, $material, "{".$common_string."}");
      $text .= "   $matvar.side = THREE.DoubleSide;\n";
      $text .= "   $matvar.transparent = true;\n";
      return $text;
   }
}



sub header {
   return <<"%"
   var obj = new THREE.Object3D();
   var allpoints = [];
%
}

sub trailer {
   return <<"%"
   scene.add(obj);
   all_objects.push(obj);

%
}

sub coordsToString {
   my ($self) = @_;
   my $text = "";

   my @coords = Utils::pointCoords($self);
   foreach (@coords) {
      $text .= $self->newCoord($_);
   }
   return $text."\n";
}

sub pointsToString {
   my ($self, $var)=@_;

   my $text = $self->coordsToString();

   my $labels = $self->source->VertexLabels;

   if ($self->source->VertexStyle !~ $Visual::hidden_re){
      my $n = scalar(@{$self->source->Vertices});

      my $thickness = $self->source->VertexThickness || 1;


      $text .= $self->newMaterial("points", "Vertex");

      $text .= "\n   <!-- POINTS -->\n";

      my $i = 0;
      while ($i < $n){
         $text .= $self->newPoint($var, $thickness, $i++);
      }
      if ($labels !~ $Visual::hidden_re && $labels != "") {
         my $i=-1;
         while ($i < $n-1){
            if (defined(my $label = $labels->(++$i))) {
               $text .= $self->newLabel($i, $label);
            }
         }
      }

      $text .= "\n";
   }

   $text .= <<"%";
   for (index = 0; index < allpoints.length; ++index) {
      allpoints[index].add(obj);
   }
%

   return $text;
}

sub verticesToString {
   my ($self, $var) = @_; 

   my $text = "";

   my $n = $self->source->Vertices->rows;
   $text .= "\n   <!-- VERTICES -->\n";
   foreach (0..$n-1) {
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

   my $text = "   var materials = [\n";

   for (my $i=0; $i<$number; ++$i) {
      $text .= $self->oneCodeMaterial($i, $material_type, $common_string, $code_props);
   }

   $text .= "   ];\n";

   $text .= <<"%";
   for (index = 0; index < materials.length; ++index) {
      materials[index].side = THREE.DoubleSide;
   }
%

   return $text .= Utils::constantMaterial($matvar, "MeshFaceMaterial", "materials");
}

sub oneCodeMaterial {
   my ($self, $index, $material_type, $common_string, $code_props) = @_;

   my $text = "      new THREE.".$material_type."({ ".$common_string;

   foreach (@$code_props) {
      $text .= Utils::writeDecor($_, $self->source->$_->($index));
   }

   $text .= "}),\n";

   return $text;
}

sub edgeMaterial {
   my ($self, $matvar, $material_type, $common_string, $code_props) = @_;

   my $text = "      var $matvar = new THREE.".$material_type."({ " . $common_string;

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

sub newArrowHelper {
   my ($self, $var, $f, $t, $length, $color, $ahead_length, $ahead_width) = @_;
   return <<"%"
   var length = allpoints[$t].vector.distanceTo(allpoints[$f].vector)-allpoints[$t].sphere.geometry.parameters.radius;
   var direction = allpoints[$t].vector.clone()
   direction.sub(allpoints[$f].vector)
   direction.normalize();
   var $var = new THREE.ArrowHelper(direction, allpoints[$f].vector, length, $color, $ahead_length*length, $ahead_width);
	obj.add($var);
%
}

sub newEdge {
   my ($self,$var,$edge,$linewidth,$color,$label,$arrow)=@_; 
   my ($f, $t) = $arrow!=-1 ? @$edge : reverse @$edge;
   my $length = 1;
	my $arrowheadlength = $ThreeJS::default::arrowheadlength;
	my $arrowheadwidth = $ThreeJS::default::arrowheadwidth;
	if ($arrow==1) {
		return $self->newArrowHelper($var, $f, $t, $length, $color, $arrowheadlength, $arrowheadwidth);
   } elsif ($arrow==-1) {
		
	} else {
		return $self->newGeometry($var) . $self->newVertex($var, $f) . $self->newVertex($var, $t) . $self->newLine($var);
	}
}

sub linesToString {
   my ($self, $var)=@_;
	my $points_thickness = $ThreeJS::default::points_thickness;
	my $lines_thickness = $ThreeJS::default::lines_thickness;
# for arrows in directed graphs: keep undef for the same color as the edge

	my $G = $self->source;
	my $thickness = $G->EdgeThickness // 1;
   my $style = $G->EdgeStyle;
   my $labels = $G->EdgeLabels;
   my $color = $G->EdgeColor;
   my $static_color; 
   if (defined($color) && !is_code($color)) {
      $static_color=$color;
   }
   my $arrowstyle=$G->ArrowStyle;
   my $text ="";

   if ($style !~ $Visual::hidden_re) {
      $text .= $self->newMaterial($var, "Edge");
      $text .= "\n   <!-- EDGES -->\n";

      for (my $e=$self->source->all_edges; $e; ++$e) {
			my $thick = (is_code($thickness)) ? $thickness->($e) : $thickness;
			my $line_width = $thick * $lines_thickness;
			my $label = defined($labels) ? $labels->($e) : undef;
			next unless $line_width || $label;
			my $col = $static_color // $color->($e) // Visual::get_RGB($Visual::Color::edges);
			my @col = split ' ', $col;
			$col = Utils::rgbToHex(@col);
			my $arrow_dir = is_code($arrowstyle) ? $arrowstyle->($e) : $arrowstyle;      
			$text .= $self->newEdge($var,$e,$line_width,$col,$label,$arrow_dir);
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

      for (my $facet = 0; $facet<@$facets; ++$facet) {
         $text .= $self->newGeometry($var);

         foreach (@{$facets->[$facet]}) {
            $text .= $self->newVertex($var,$_);
         }
         # first vertex again
         $text .= $self->newVertex($var, $facets->[$facet]->[0]);
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


sub writeDecor {
   my ($name, $value) = @_;

   if ($name =~ "Color") {
      return "color: " . Utils::rgbToHex(@$value) . ", ";
   }

   if ($name eq "FacetTransparency") {
      my $opacity = defined($value) ? 1-$value : 1;
      if ($opacity > 0.5) {
         return "transparent: true, opacity: $opacity, side: THREE.DoubleSide , depthWrite: true, depthTest: true, ";
      } else {
         return "transparent: true, opacity: $opacity, side: THREE.DoubleSide , depthWrite: false, depthTest: false, ";
      }
   }

   if ($name eq "EdgeThickness") {
      my $width = $value || 1.5;
      return "linewidth: $width, ";
   }
}


sub constantMaterial {
   my ($matvar, $material_type, $material_string) = @_;

   return << "%";
   var $matvar = new THREE.$material_type ( $material_string );

%
}

sub convertExtraData {
   my ($name,$data) = @_;
   return "   var $name = [". join(",\n      ", map {ref $_ eq "ARRAY" ? "[".join(",",@$_)."]" : $_ } @$data)."];\n\n";

}

1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
