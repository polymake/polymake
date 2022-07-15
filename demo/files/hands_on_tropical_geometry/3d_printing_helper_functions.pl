#!/usr/bin/perl

use application "fan";

sub generateBoundingBox
{
    my $V = $_[0];

    my $xmargin = 1;
    my $ymargin = 1;
    my $zmargin = 1;
    if (defined $_[1])
    {
        $xmargin = $_[1];
    }
    if (defined $_[2])
    {
        $ymargin = $_[2];
    }
    if (defined $_[3])
    {
        $zmargin = $_[3];
    }

    # finding the minimal and maximal x,y,z values of all vertices
    my $W = $V->VERTICES;
    my $xmin = 0;
    my $xmax = 0;
    my $ymin = 0;
    my $ymax = 0;
    my $zmin = 0;
    my $zmax = 0;

    for (my $i=0; $i<$W->rows; ++$i)
    {
        my $w = $W->row($i);
        if ($w->[0]>0)
        {
            my $xvalue = $w->[1]/$w->[0];
            my $yvalue = $w->[2]/$w->[0];
            my $zvalue = $w->[3]/$w->[0];
            $xmin = min($xmin,$xvalue);
            $xmax = max($xmax,$xvalue);
            $ymin = min($ymin,$yvalue);
            $ymax = max($ymax,$yvalue);
            $zmin = min($zmin,$zvalue);
            $zmax = max($zmax,$zvalue);
        }
    }

    # return box AROUND the vertices with margin
    $xmin = $xmin-$xmargin;
    $xmax = $xmax+$xmargin;
    $ymin = $ymin-$ymargin;
    $ymax = $ymax+$ymargin;
    $zmin = $zmin-$zmargin;
    $zmax = $zmax+$zmargin;

    my $boundingBox = new Polytope<Rational>(POINTS=>[[1,$xmin,$ymin,$zmin],
                                                      [1,$xmax,$ymin,$zmin],
                                                      [1,$xmin,$ymax,$zmin],
                                                      [1,$xmin,$ymin,$zmax],
                                                      [1,$xmax,$ymax,$zmin],
                                                      [1,$xmax,$ymin,$zmax],
                                                      [1,$xmin,$ymax,$zmax],
                                                      [1,$xmax,$ymax,$zmax]]);
    return $boundingBox;
}


sub intersectWithBoundingBox
{
    my $V = $_[0];
    my $boundingBox = $_[1];

    my $boundingBoxFan = check_fan_objects($boundingBox);
    my $VBoundedFan = common_refinement($V,$boundingBoxFan);

    my $VBounded = new PolyhedralComplex(VERTICES=>$VBoundedFan->RAYS,
                                         MAXIMAL_POLYTOPES=>$VBoundedFan->MAXIMAL_CONES);
    return $VBounded;
}


sub intersectWithBoundingBoxForFraming
{
    my $V = $_[0];
    my $boundingBox = $_[1];

    my $boundingBoxFan = check_fan_objects($boundingBox);
    my $boundingBoxSkeletonFan = new fan::PolyhedralFan(RAYS=>$boundingBoxFan->RAYS, MAXIMAL_CONES=>$boundingBoxFan->CONES->[2]);
    my $VFrameFan = common_refinement($V,$boundingBoxSkeletonFan);

    my $VFrame = new fan::PolyhedralComplex(VERTICES=>$VFrameFan->RAYS, MAXIMAL_POLYTOPES=>$VFrameFan->MAXIMAL_CONES);
    return $VFrame;
}


#####
# Functions for generating .scad files
#####
sub generateSCADFileForSurface
{
    my $tropicalSurface = $_[0];
    my $filename = $_[1];

    my $pointsSurface = $tropicalSurface->VERTICES->minor(All,~[0]);
    my $edgesSurface = $tropicalSurface->POLYTOPES->[1];
    my $facesSurface = $tropicalSurface->POLYTOPES->[2];



    my $pointsSurfaceString = "pointsSurface = scalingFactor*[";
    for (my $i=0; $i<$pointsSurface->rows; ++$i)
    {
        my $p = $pointsSurface->row($i);
        $pointsSurfaceString = $pointsSurfaceString . "[";
        for (my $j=0; $j<$p->dim; ++$j)
        {
            $pointsSurfaceString = $pointsSurfaceString . $p->[$j] . ",";
        }
        chop($pointsSurfaceString);
        $pointsSurfaceString = $pointsSurfaceString . "],";
    }
    chop($pointsSurfaceString);
    $pointsSurfaceString = $pointsSurfaceString . "];";

    my $edgesSurfaceString = "edgesSurface = [";
    foreach (@$edgesSurface)
    {
        $edgesSurfaceString = $edgesSurfaceString . "[" . join(",",@$_) . "],";
    }
    chop($edgesSurfaceString);
    $edgesSurfaceString = $edgesSurfaceString . "];";

    my $facesSurfaceString = "facesSurface = [";
    foreach (@$facesSurface)
    {
        $facesSurfaceString = $facesSurfaceString . "[" . join(",",@$_) . "],";
    }
    chop($facesSurfaceString);
    $facesSurfaceString = $facesSurfaceString . "];";



my $OpenSCADFileContent =
"/**
 * VISUAL PARAMETERS - feel free to change and experiment
 *   color can be specified by name as a string or by RGB values between 0 and 1.
 *   for a list of color names, see \"HTML color names\" in
 *   https://en.wikipedia.org/wiki/Web_colors
 **/
colorSurface = \"SlateGray\"; // color of surface
scalingFactor = 1; // global scaling factor
thicknessSurface = 0.1; // thickness of surface



/**
 * DATA PRODUCED BY POLYMAKE - change at your own risk!!!
 **/\n";
$OpenSCADFileContent = $OpenSCADFileContent
    . $pointsSurfaceString . "\n"
    . $edgesSurfaceString . "\n"
    . $facesSurfaceString . "\n\n\n";
$OpenSCADFileContent = $OpenSCADFileContent .
"/**
 * DRAWING FUNCTIONS - change at your own risk!!!
 **/
module minkhullind(face,pts,r)
{
  minkowski()
  {
    hull()
    {
      for (i = [0:len(face)-1])
        translate(pts[face[i]])  sphere(.001);
    };
    sphere(0.5*r);
  };
}

union()
{
  for(j=[0:len(facesSurface)-1])
    color(colorSurface) minkhullind(facesSurface[j],pointsSurface,thicknessSurface);
};";

    open(FH, '>', $filename) or die $!;
    print FH $OpenSCADFileContent;
    close(FH);
}


sub generateSCADFileForSurfaceAndCurve
{
    my $tropicalSurface = $_[0];
    my $tropicalCurve = $_[1];
    my $filename = $_[2];

    my $pointsSurface = $tropicalSurface->VERTICES->minor(All,~[0]);
    my $edgesSurface = $tropicalSurface->POLYTOPES->[1];
    my $facesSurface = $tropicalSurface->POLYTOPES->[2];

    my $pointsCurve = $tropicalCurve->VERTICES->minor(All,~[0]);
    my $edgesCurve = $tropicalCurve->POLYTOPES->[1];



    my $pointsSurfaceString = "pointsSurface = scalingFactor*[";
    for (my $i=0; $i<$pointsSurface->rows; ++$i)
    {
        my $p = $pointsSurface->row($i);
        $pointsSurfaceString = $pointsSurfaceString . "[";
        for (my $j=0; $j<$p->dim; ++$j)
        {
            $pointsSurfaceString = $pointsSurfaceString . $p->[$j] . ",";
        }
        chop($pointsSurfaceString);
        $pointsSurfaceString = $pointsSurfaceString . "],";
    }
    chop($pointsSurfaceString);
    $pointsSurfaceString = $pointsSurfaceString . "];";

    my $edgesSurfaceString = "edgesSurface = [";
    foreach (@$edgesSurface)
    {
        $edgesSurfaceString = $edgesSurfaceString . "[" . join(",",@$_) . "],";
    }
    chop($edgesSurfaceString);
    $edgesSurfaceString = $edgesSurfaceString . "];";

    my $facesSurfaceString = "facesSurface = [";
    foreach (@$facesSurface)
    {
        $facesSurfaceString = $facesSurfaceString . "[" . join(",",@$_) . "],";
    }
    chop($facesSurfaceString);
    $facesSurfaceString = $facesSurfaceString . "];";

    my $pointsCurveString = "pointsCurve = scalingFactor*[";
    for (my $i=0; $i<$pointsCurve->rows; ++$i)
    {
        my $p = $pointsCurve->row($i);
        $pointsCurveString = $pointsCurveString . "[";
        for (my $j=0; $j<$p->dim; ++$j)
        {
            $pointsCurveString = $pointsCurveString . $p->[$j] . ",";
        }
        chop($pointsCurveString);
        $pointsCurveString = $pointsCurveString . "],";
    }
    chop($pointsCurveString);
    $pointsCurveString = $pointsCurveString . "];";

    my $edgesCurveString = "edgesCurve = [";
    foreach (@$edgesCurve)
    {
        $edgesCurveString = $edgesCurveString . "[" . join(",",@$_) . "],";
    }
    chop($edgesCurveString);
    $edgesCurveString = $edgesCurveString . "];";



my $OpenSCADFileContent =
"/**
 * VISUAL PARAMETERS - feel free to change and experiment
 *   color can be specified by name as a string or by RGB values between 0 and 1.
 *   for a list of color names, see \"HTML color names\" in
 *   https://en.wikipedia.org/wiki/Web_colors
 **/
colorSurface = \"SlateGray\"; // color of surface
colorCurve = [0.83,.15,0.27]; // color of curve
scalingFactor = 30; // global scaling factor
thicknessSurface = 0.3; // thickness of surface
thicknessCurve = 2; // thickness of curve



/**
 * DATA PRODUCED BY POLYMAKE - change at your own risk!!!
 **/\n";
$OpenSCADFileContent = $OpenSCADFileContent
    . $pointsSurfaceString . "\n"
    . $edgesSurfaceString . "\n"
    . $facesSurfaceString . "\n"
    . $pointsCurveString . "\n"
    . $edgesCurveString . "\n\n\n";
$OpenSCADFileContent = $OpenSCADFileContent .
"/**
 * DRAWING FUNCTIONS - change at your own risk!!!
 **/
module minkhullind(face,pts,r)
{
  minkowski()
  {
    hull()
    {
      for (i = [0:len(face)-1])
        translate(pts[face[i]])  sphere(.1);
    };
    sphere(r);
  };
}

union()
{
  difference()
  {
    for(j=[0:len(facesSurface)-1])
      color(colorSurface) minkhullind(facesSurface[j],pointsSurface,thicknessSurface);

    for(j=[0:len(edgesCurve)-1])
      color(colorCurve) minkhullind(edgesCurve[j],pointsCurve,thicknessCurve);
  }

  for(j=[0:len(edgesCurve)-1])
    color(colorCurve) minkhullind(edgesCurve[j],pointsCurve,thicknessCurve);
};";

    open(FH, '>', $filename) or die $!;
    print FH $OpenSCADFileContent;
    close(FH);
}

sub generateSCADFileForCurve
{
    my $tropicalFrame = $_[0];
    my $tropicalCurve = $_[1];
    my $filename = $_[2];

    my $pointsFrame = $tropicalFrame->VERTICES->minor(All,~[0]);
    my $edgesFrame = $tropicalFrame->POLYTOPES->[1];

    my $pointsCurve = $tropicalCurve->VERTICES->minor(All,~[0]);
    my $edgesCurve = $tropicalCurve->POLYTOPES->[1];



    my $pointsFrameString = "pointsFrame = scalingFactor*[";
    for (my $i=0; $i<$pointsFrame->rows; ++$i)
    {
        my $p = $pointsFrame->row($i);
        $pointsFrameString = $pointsFrameString . "[";
        for (my $j=0; $j<$p->dim; ++$j)
        {
            $pointsFrameString = $pointsFrameString . $p->[$j] . ",";
        }
        chop($pointsFrameString);
        $pointsFrameString = $pointsFrameString . "],";
    }
    chop($pointsFrameString);
    $pointsFrameString = $pointsFrameString . "];";

    my $edgesFrameString = "edgesFrame = [";
    foreach (@$edgesFrame)
    {
        $edgesFrameString = $edgesFrameString . "[" . join(",",@$_) . "],";
    }
    chop($edgesFrameString);
    $edgesFrameString = $edgesFrameString . "];";

    my $pointsCurveString = "pointsCurve = scalingFactor*[";
    for (my $i=0; $i<$pointsCurve->rows; ++$i)
    {
        my $p = $pointsCurve->row($i);
        $pointsCurveString = $pointsCurveString . "[";
        for (my $j=0; $j<$p->dim; ++$j)
        {
            $pointsCurveString = $pointsCurveString . $p->[$j] . ",";
        }
        chop($pointsCurveString);
        $pointsCurveString = $pointsCurveString . "],";
    }
    chop($pointsCurveString);
    $pointsCurveString = $pointsCurveString . "];";

    my $edgesCurveString = "edgesCurve = [";
    foreach (@$edgesCurve)
    {
        $edgesCurveString = $edgesCurveString . "[" . join(",",@$_) . "],";
    }
    chop($edgesCurveString);
    $edgesCurveString = $edgesCurveString . "];";



my $OpenSCADFileContent =
"/**
 * VISUAL PARAMETERS - feel free to change and experiment
 *   color can be specified by name as a string or by RGB values between 0 and 1.
 *   for a list of color names, see \"HTML color names\" in
 *   https://en.wikipedia.org/wiki/Web_colors
 **/
colorFrame = \"SlateGray\"; // color of frame
colorCurve = [0.83,.15,0.27]; // color of curve
scalingFactor = 1; // global scaling factor
thicknessFrame = 0.05; // thickness of frame
thicknessCurve = 0.05; // thickness of curve



/**
 * DATA PRODUCED BY POLYMAKE - change at your own risk!!!
 **/\n";
$OpenSCADFileContent = $OpenSCADFileContent
    . $pointsFrameString . "\n"
    . $edgesFrameString . "\n"
    . $pointsCurveString . "\n"
    . $edgesCurveString . "\n\n\n";
$OpenSCADFileContent = $OpenSCADFileContent .
"/**
 * DRAWING FUNCTIONS - change at your own risk!!!
 **/
module minkhullind(face,pts,r)
{
  minkowski()
  {
    hull()
    {
      for (i = [0:len(face)-1])
        translate(pts[face[i]])  sphere(.0001);
    };
    sphere(r);
  };
}

union()
{
  difference()
  {
    for(j=[0:len(edgesFrame)-1])
      color(colorFrame) minkhullind(edgesFrame[j],pointsFrame,thicknessFrame);

    for(j=[0:len(edgesCurve)-1])
      color(colorCurve) minkhullind(edgesCurve[j],pointsCurve,thicknessCurve);
  };

  for(j=[0:len(edgesCurve)-1])
    color(colorCurve) minkhullind(edgesCurve[j],pointsCurve,thicknessCurve);
};";

    open(FH, '>', $filename) or die $!;
    print FH $OpenSCADFileContent;
    close(FH);
}



# #####
# #
# # Functions for ordering face indices for OpenSCAD according to an orientation,
# # i.e., A-----B
# #       |     |
# #       D-----C
# #       should be in the order {A, B, C, D} or {A, D, C, B}.
# #
# # NOT NECESSARY ANYMORE!!!
# #
# #####
# sub does_array_contain
# {
#     my $array = $_[0];
#     my $element = $_[1];

#     foreach (@$array)
#     {
#         if ($element == $_)
#         {
#             return 1;
#         }
#     }
#     return 0;
# }
# sub order_face
# {
#     my $face = $_[0];
#     my $edges = $_[1];

#     my $starting_vertex = $face->front();
#     my $current_vertex = $starting_vertex;
#     my $ordered_face = [$starting_vertex];
#     while (scalar(@$ordered_face)<$face->size())
#     {
#         foreach (@$edges)
#         {
#             my $e0 = $_->front();
#             my $e1 = $_->back();
#             if ($e0 == $current_vertex)
#             {
#                 if (($face->contains($e1)) and (not(does_array_contain($ordered_face,$e1))))
#                 {
#                     $current_vertex = $e1;
#                     last;
#                 }
#             }
#             if ($e1 == $current_vertex)
#             {
#                 if (($face->contains($e0)) and (not(does_array_contain($ordered_face,$e0))))
#                 {
#                     $current_vertex = $e0;
#                     last;
#                 }
#             }
#         }
#         push(@$ordered_face,$current_vertex);
#     }

#     return $ordered_face;
# }
# sub order_faces
# {
#     my $faces = $_[0];
#     my $edges = $_[1];

#     my @ordered_faces = ();

#     foreach (@$faces)
#     {
#         push(@ordered_faces,order_face($_,$edges));
#     }

#     return @ordered_faces;
# }
