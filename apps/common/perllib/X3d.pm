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

# Interface to X3d.
#
# This file only provides the basic functionality.

package X3d;

use XML::Writer;
use Math::Trig;

my $PI=2*asin(1);

#the average ratio of the glyph weight and height
declare $avg_char_width = 0.7;

################################################################################
# helping subs

#TODO some of these are probably somewhere?
sub text_extents {
   my $text = shift;
   return $avg_char_width*length($text);
}

sub embed_3d {   
   my $points = shift;
   return $points->cols < 3 ? $points|zero_matrix<Float>($points->rows,3-$points->cols) : $points;
}

# cross product
sub xp {
   my ($v,$u) = @_;
   my @v = @{$v};
   my @u = @{$u};
   return new Vector<Float>([ $v[1]*$u[2]-$v[2]*$u[1],
                              $v[2]*$u[0]-$v[0]*$u[2],
                              $v[0]*$u[1]-$v[1]*$u[0]]);
}

# norm
sub norm {
   my $v = shift;
   return sqrt(sqr($v));
}

# oriented angle w.r.t. the normal $n (-pi,pi]  
sub signed_angle {
   my ($v,$u,$n) = @_;
   if (is_zero($v) || is_zero($u)) {
      croak("X3d sub signed_angle:  no zero vectors allowed");
   }
   my $xp = xp($v,$u);
   my $sgn = $xp*$n <=> 0;
   my $alpha = $sgn*acos($v*$u)/(norm($v)*norm($u));
   return $alpha;
}

################################################################################
#
#  an abstract base class for various drawables (Graph, Polygon, Gale, etc.)
#
package X3d::Element;
use Polymake::Struct (
   [ '$viewpoint_orientation' => 'undef' ], # this is calculated by the Viewpoint sub, could be used to rotate labels for blender since there are no billboards
   [ '$bbox' => 'undef' ],
);

sub draw_label {
   my ($self,$x,$label,$offset,$color,$transparency)=@_;
   $transparency=$transparency // 0;
   $x->startTag('Transform',  translation => Visual::print_coords($offset), 
                              rotation    => $self->viewpoint_orientation);
      $x->startTag('Billboard'); # billboards are ignored by the blender import, labels are facing in viewpoint direction 
         $x->startTag('Shape');
            $x->startTag('Text', string => '"'.$label.'"');
               $x->emptyTag('FontStyle',  size => $scale*$font_size, 
                                          family => "\"".$font_family."\"",
                                          style => "PLAIN",
                                          justify => "\"MIDDLE\"");
            $x->endTag('Text');
            $x->startTag('Appearance');
               $x->emptyTag('Material', diffuseColor=>$color, transparency => $transparency);
            $x->endTag('Appearance');
         $x->endTag('Shape');
      $x->endTag('Billboard');
   $x->endTag('Transform');
}


################################################################################

package X3d::File;


use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   '@geometries',
   [ '$unnamed' => '0' ],
   [ '$title' => '#1' ],
);

sub coord_axes {
   my ($self,$x)=@_;
   my $ahead_l = $arrowhead_length * $scale; 
   my $ahead_r = $arrowhead_radius * $scale;
   my $cyl_r = $edge_radius * $scale * 0.8;
   my $cyl_h = 4;
   my $ahead_tr = new Vector<Float>([0, ($cyl_h+$ahead_l)/2, 0]);
   my @colors = ('1 0 0','0 0 1','0 1 0');   # blenders coord axes colors
   my $transparency = 0.7;
   my @perm = (2,1,0);

   $x->startTag('Group');
   foreach (0..2) {
      my $raxis = unit_vector<Float>(3,$perm[$_]);
      $x->startTag('Transform',  rotation=> Visual::print_coords($raxis)." ".(-1+$_)*$PI/2);
         $x->startTag('Shape');
   	      $x->startTag('Appearance');
   			   $x->emptyTag('Material', diffuseColor => $colors[$_], transparency => $transparency);
   			$x->endTag('Appearance');
   			$x->emptyTag('Cylinder',   radius => $cyl_r,
   			                  		   height => $cyl_h);
   		$x->endTag('Shape');
   
         $x->startTag('Transform',  translation => Visual::print_coords($ahead_tr)); 
   	      $x->startTag('Shape');
   		      $x->startTag('Appearance');
   			      $x->emptyTag('Material', diffuseColor => $colors[$_], transparency => $transparency);
   			   $x->endTag('Appearance');
   			   $x->emptyTag('Cone', bottomRadius => $ahead_r,
   			                  		height => $ahead_l);
   		   $x->endTag('Shape');
   	   $x->endTag('Transform');
      $x->endTag('Transform');
      my $label_offset = ($cyl_h/2+$ahead_l)*unit_vector<Float>(3,$_);

      $self->draw_label($x,$axeslabels[$_],$label_offset,$colors[$_],$transparency);
   }
   $x->endTag('Group');
}

sub WorldInfo {
   my ($self,$x)=@_;
   my $title = $self->title // 'unnamed';
   $x->emptyTag('WorldInfo', title => $title)
}

sub Viewpoint {
   my ($self,$x)=@_;
   my $oo = new Vector<Float>([0,0,1]);         # thats the original viewpoint orientation we have to rotate 
   my $vp = new Vector<Float>([@view_point]);
   my $va = new Vector<Float>([@view_at]);
   my $vu = new Vector<Float>([@view_up]);  
   $vu = $vu/norm($vu);
   my $v = $vp-$va;                             # new view direction
   $v = $v/norm($v);
   my $q = $vu-($vu*$v)*$v;                     # new view_up perpendicular to the view direction
   
   $q = $q/norm($q);
   my $p = xp($q,$v);
   if (is_zero($p)) {
      print "X3d message: view_up and view_point-view_at are colinear, view orientation is not uniquely defined\n";
      $q = normalized(null_space($v))->row(0);
      $p = xp($q,$v);   
   }
   $p = $p/norm($p);
   my $R = new Matrix<Float>([$p,$q,$v]);
   my $rotaxis = normalized(null_space($R-unit_matrix<Float>(3)))->row(0);
   my $normal = normalized(null_space($rotaxis))->row(0);
   my $alpha = signed_angle($normal,$normal*$R,$rotaxis);
   $self->viewpoint_orientation = Visual::print_coords($rotaxis)." ".$alpha;
   $x->emptyTag('Viewpoint',  description => 'Start',
                              position    => Visual::print_coords($vp), 
                              orientation => $self->viewpoint_orientation,
                              fieldOfView => $PI/6, );
}

sub Background {
   my ($self,$x)=@_;
   # blender import ignores this
   $x->emptyTag('Background', skyColor => '1 1 1', groundColor => '1 1 1', )
}
  
sub NavigationInfo {
   my ($self,$x)=@_;
}

sub init_scene {
   my ($self,$x)=@_;
   $self->WorldInfo($x);
   $self->Viewpoint($x);
   $self->Background($x);
   $self->coord_axes($x) unless !$coordinate_axes;
}

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

sub toString {
   my ($self)=@_;
   my $title = $self->title // "unnamed";
   
   my $x = XML::Writer->new( OUTPUT => 'self', DATA_MODE => 1, DATA_INDENT => 3 );

   $x->xmlDecl('UTF-8');
   $x->doctype('X3D', "ISO//Web3D//DTD X3D 3.0//EN", "http://www.web3d.org/specifications/x3d-3.0.dtd");
   $x->startTag('X3D', profile => "Interchange", version => "3.0", 'xmlns:xsd' => "http://www.w3.org/2001/XMLSchema-instance", 'xsd:noNamespaceSchemaLocation' => "http://www.web3d.org/specifications/x3d-3.0.xsd");
   $x->startTag('head');
   $x->emptyTag('meta', name => "generator", content => "Polymake"); 
   $x->emptyTag('meta', name => "source", content => $title);
   $x->endTag('head'); 
   $x->startTag('Scene');
   $self->init_scene($x);
  
   foreach (@{$self->geometries}) {
      $_->viewpoint_orientation = $self->viewpoint_orientation;
      $x->startTag('Group');
      $_->draw($x);
      $x->endTag('Group'); 
   }
   $x->endTag('Scene'); 
   $x->endTag('X3D');
   $x->end;
   my $xml = $x->to_string;
   return $xml;
}

###########################################################################
#
#  Basis class for all graphical objects handled by x3d
#

package X3d::PointSet;

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
   '$coords',
   '@radii',
   '@label_widths',
);

sub init {
   my $self = shift;
   my $P = $self->source;
   $self->coords = embed_3d($P->Vertices); #TODO reuse?: that sub is probably somewhere
   my $thickness = $P->VertexThickness;
   my $labels = $P->VertexLabels;
   my $style = $P->VertexStyle;
   foreach (0..scalar(@{$P->Vertices})-1) {
      my $thick = is_code($thickness) ? $thickness->($_) : $thickness;
      my $radius = ($style =~ $Visual::hidden_re) ? 0 : $scale * $point_radius * $thick;
      push @{$self->radii}, $radius;
      my $label = defined($labels) ? $labels->($_) : undef;
      push @{$self->label_widths}, $scale * ($avg_char_width * $font_size * length($label) + $text_spacing ) + $radius;
   }
}

sub new {
   my $self=&_new;
   $self->init;
   $self;
}

sub draw_def {
   my ($self,$writedefto,$x)=@_;
   my $P=$self->source;
   
   # NOTES ON THE BLENDER IMPORT:
   # does not resolve prototypes properly
   # strangely reverts the X axis...
   
   # quote /usr/share/blender/scripts/addons/io_scene_x3d/import_x3d.py    line 1878  ff:
   #     # we need unflattened coord array here, while
   #     # importmesh_readvertices uses flattened. can't cache both :(
   #     # todo: resolve that somehow, so that vertex set can be effectively
   #     # reused between different mesh types?
   # end of quote
   # thus, we can't use a coordinate def in, for example, a pointset if we want to 
   # use it in an indexedfaceset
   
   $x->startTag('Shape');
		$x->startTag($writedefto, solid => "false");
			$x->emptyTag('Coordinate', DEF => "pointCoords", 
			                           point => join(' ', map { Visual::print_coords($_) } @{$self->coords}));
		$x->endTag($writedefto);
	$x->endTag('Shape');
}


sub draw_point {
   my ($self,$x,$coord,$radius,$color,$label)=@_;

   $x->startTag('Transform', translation => Visual::print_coords($coord));
   #the vertex sphere
   if ($radius) {
      $x->startTag('Shape');
         $x->startTag('Sphere', radius => $radius);
         $x->endTag('Sphere');
         $x->startTag('Appearance');
            $x->emptyTag('Material', diffuseColor => $color);
         $x->endTag('Appearance');
      $x->endTag('Shape');
   }
   my $label_offset = new Vector<Float>([0,$radius+$text_spacing*$scale,0]);
   if (defined($label) && $label!~/^\s*$/) {
      $self->draw_label($x,$label,$label_offset,'0 0 0');
   }
   $x->endTag('Transform');
}

sub draw_edge {
   my ($self,$x,$edge,$cyl_r,$edge_color,$label,$arrow_dir,$ahead_col)=@_; 
   my ($f, $t) = $arrow_dir!=-1 ? @$edge : reverse @$edge;
	
	my $ahead_l = (!$arrow_dir) ? 0 : $arrowhead_length * $scale; 
	my $ahead_r = $arrowhead_radius * $scale;
   
   if ($ahead_l || !$use_lines || defined($label)) {
		my $fr = @{$self->coords}[$f];
		my $to = @{$self->coords}[$t];
		my $to_r = @{$self->radii}[$t];
		my $fr_r = @{$self->radii}[$f];
		my $to_offset = $ahead_l ? $to_r : sqrt(max(0,$to_r**2-$cyl_r**2));
		my $fr_offset = sqrt(max(0,$fr_r**2-$cyl_r**2));
	
		my $diff = $to - $fr;
		my $dist =  norm($diff);
		my $length = $dist - $fr_offset - $to_offset;
		my $cyl_h = $length - $ahead_l;
		my $min_y = $cyl_h/2;
		
		my $q = ($min_y + $fr_offset)/$dist;	
		my $translation = $fr + $q * $diff;
	
		my $rot_from = new Vector<Float>([0, - $min_y - $fr_offset, 0]);
		my $rot_to = $q * (-$diff);
		my $rot_axis = ($rot_from + $rot_to)/2;
		my $label_offset = new Vector<Float>([0,0,0]);
	
		$x->startTag('Transform',  translation => Visual::print_coords($translation)); 
			if ($ahead_l || !$use_lines) {
				$x->startTag('Transform',  rotation=> Visual::print_coords($rot_axis)." ".$PI);
				if ($ahead_l) {
					my $ahead_translation = new Vector<Float>([0, ($cyl_h+$ahead_l)/2, 0]);
					$x->startTag('Transform',  translation => Visual::print_coords($ahead_translation)); 
						$x->startTag('Shape');
							$x->startTag('Appearance');
								$x->emptyTag('Material', diffuseColor => $ahead_col);
							$x->endTag('Appearance');
							$x->emptyTag('Cone', bottomRadius => $ahead_r,
														height => $ahead_l);
						$x->endTag('Shape');
					$x->endTag('Transform');
				}

				if (!$use_lines) {
						$x->startTag('Shape');
						$x->startTag('Appearance');
							$x->emptyTag('Material', diffuseColor => $edge_color);
						$x->endTag('Appearance');
						$x->emptyTag('Cylinder',   radius => $cyl_r,
															height => $cyl_h);
					$x->endTag('Shape');
				}
			}
			$x->endTag('Transform');

		if (defined($label) && $label!~/^\s*$/) {
			$self->draw_label($x,$label,$label_offset,'0 0 0');
		}
		$x->endTag('Transform');
	}	
	if ($use_lines) {
		$x->startTag('Shape');
			$x->startTag('Appearance');
				$x->emptyTag('Material', diffuseColor => $edge_color);
			$x->endTag('Appearance');
			$x->startTag('IndexedLineSet', coordIndex => $f." ".$t);
				$x->emptyTag('Coordinate', USE => "pointCoords");
			$x->endTag('IndexedLineSet');
		$x->endTag('Shape');
	}

}

sub draw_points {
   my ($self,$x)=@_;
   my $P = $self->source;
   
   # decor
   my $thickness = $P->VertexThickness // 1;
   my $color = $P->VertexColor;
   my $labels = $P->VertexLabels;
   my $static_color; 
   if (defined($color) && !is_code($color)) {
      $static_color = $color;
   }
   
   foreach my $i (0..scalar(@{$self->coords})-1) {
      my $col = $static_color // $color->($i) // $Visual::Color::vertices;
      my $label = defined($labels) ? $labels->($i) : undef;
      next unless defined($label) || @{$self->radii}[$i];
      $col = $col->toFloat;
      $self->draw_point($x,@{$self->coords}[$i],@{$self->radii}[$i],$col,$label);  
   }
}

sub draw {
   my ($self,$x)=@_;
   $self->draw_points($x) unless $self->source->VertexStyle =~ $Visual::hidden_re;
   $x;
}


###########################################################################
package X3d::Solid;

use Polymake::Struct [ '@ISA' => 'PointSet' ];


sub draw_facet {
   my ($self,$x,$facet,$color,$transparency,$label)=@_;
   $x->startTag('Shape');
      $x->startTag('Appearance');
		   $x->emptyTag('Material', diffuseColor => $color, transparency => $transparency);
		$x->endTag('Appearance');
		$x->startTag('IndexedFaceSet', solid => "false", coordIndex => $facet);
			$x->emptyTag('Coordinate', USE => "pointCoords");
		$x->endTag('IndexedFaceSet');
	$x->endTag('Shape');
   
   my $label_offset = new Vector<Float>([0,0,0]);
   if (defined($label) && $label!~/^\s*$/) {
      $self->draw_label($x,$label,$label_offset,'0 0 0');
   }

}

sub draw_facets {
   my ($self,$x)=@_;
   my $P=$self->source;
     
   my @facets = map { sprintf $_ } @{$P->Facets};

   # decor
   my $style = $P->FacetStyle;
   my $labels = $P->FacetLabels;
   my $transparency = $P->FacetTransparency // 0;
   my $color = $P->FacetColor;
   my $static_color; 
   if (defined($color) && !is_code($color)) {
      $static_color=$color;
   }

   foreach my $i (0..scalar(@facets)-1) {
      my $col = $static_color // $color->($i) // $Visual::Color::facets;
      my $tp = (is_code($transparency)) ? $transparency->($i) : $transparency;
      my $label = defined($labels) ? $labels->($i) : undef;
      $col = $col->toFloat;
      $self->draw_facet($x,$facets[$i],$col,$tp,$label);  
   }
}

sub draw_edges {
   my ($self,$x)=@_;
   my $P=$self->source;
   
   #decor
   my $thickness = $P->EdgeThickness // 1;
   return unless $thickness;
   my $style = $P->EdgeStyle;
   my $color = $P->EdgeColor // Visual::get_RGB($Visual::Color::edges);
   $color = $color->toFloat;

   my $radius = $scale * $edge_radius * $thickness; 
   
   # there is probably a better way to get an edge iterator?
   my $G = graph::graph_from_cycles($P->Facets);
   foreach my $edge (@{$G->EDGES}) {
      $self->draw_edge($x,$edge,$radius,$color,undef,0,'0 0 0');
   }
}

sub draw {
   my ($self,$x)=@_;
   $self->draw_points($x) unless $self->source->VertexStyle =~ $Visual::hidden_re;
   $self->draw_def('IndexedFaceSet',$x);
   if ($use_lines) { #this is just because the blender import script limps
   	$self->draw_def('IndexedLineSet',$x); 
   }
   $self->draw_edges($x) unless $self->source->EdgeStyle =~ $Visual::hidden_re;
   $self->draw_facets($x) unless $self->source->FacetStyle =~ $Visual::hidden_re;
   $x;
}
    

      
##############################################################################################
#
#  Wire model (e.g. a graph)

package X3d::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);


sub draw_edges {
   my ($self,$x)=@_;
   my $G=$self->source;
   
   # decor
   my $thickness = $G->EdgeThickness // 1;
   my $style = $G->EdgeStyle;
   my $labels = $G->EdgeLabels;
   my $color = $G->EdgeColor;
   my $static_color; 
   if (defined($color) && !is_code($color)) {
      $static_color=$color;
   }
   my $arrowstyle=$G->ArrowStyle;

   for (my $e=$G->all_edges; $e; ++$e) {
      my $thick = (is_code($thickness)) ? $thickness->($e) : $thickness;
      my $radius = $scale * $edge_radius * $thick;
      my $label = defined($labels) ? $labels->($e) : undef;
      next unless $radius || $label;
      my $col = $static_color // $color->($e) // Visual::get_RGB($Visual::Color::edges);
      $col = $col->toFloat;
      my $arrow_dir = is_code($arrowstyle) ? $arrowstyle->($e) : $arrowstyle;      
      my $arrow_col = $arrowheadcolor // $col;
      $self->draw_edge($x,$e,$radius,$col,$label,$arrow_dir,$arrow_col);
   }
}

sub draw {
   my ($self,$x)=@_;
   if ($use_lines) { 
   	$self->draw_def('IndexedLineSet',$x); 
   }
   $self->draw_points($x) unless $self->source->VertexStyle =~ $Visual::hidden_re;
   $self->draw_edges($x) unless $self->source->EdgeStyle =~ $Visual::hidden_re;
   $x;
}

1;

# Local Variables:
# c-basic-offset:3
# End:

