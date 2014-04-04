# This file, test.rb, is an example to show how to use jruby (jruby.codehaus.org) with jReality.  
#
#To run this example with the JOGL backend enabled:
#1) download jruby (jruby.codehaus.org) 
#2) setenv CLASSPATH  to include the jarfiles jReality.jar, beans.jar, and jogl.jar
#3) setenv JAVA_LIBRARY_PATH to include the native libraries for jogl.
#4) run "jruby test.rb"
#
#If successful, you should see a textured cylinder displayed in a ViewerApp application.
require 'java' 
import 'java.io.IOException' 
import 'java.net.URL' 
import 'java.awt.Color' 

import 'de.jreality.geometry.Primitives' 
import 'de.jreality.reader.Readers' 
import 'de.jreality.plugin.JRViewer' 
import 'de.jreality.scene.Viewer' 
import 'de.jreality.scene.data.AttributeEntityUtility' 

class MainWindow 
include_package 'de.jreality.shader' 
include_package 'de.jreality.util' 
include_package 'de.jreality.scene' 
include_package 'de.jreality.math' 

def initialize 
# Scene, Viewerapp setting 
  myscene = SceneGraphUtility.createFullSceneGraphComponent("myscene") 
  myscene.setGeometry(Primitives.cylinder(20)) 
  ap = myscene.getAppearance 
# Geometry setting 
  dgs = ShaderUtility.createDefaultGeometryShader(ap, true) 
  dgs.setShowLines(false) 
  dgs.setShowPoints(false) 
  dps = dgs.createPolygonShader("default") 
  dps.setDiffuseColor(Color.white) 
# Texture setting with matrix scaling 
  tex2d = AttributeEntityUtility.createAttributeEntity(Texture2D.java_class, CommonAttributes::POLYGON_SHADER + "." + CommonAttributes::TEXTURE_2D,ap, true) 
  is = URL.new("http://www3.math.tu-berlin.de/jreality/download/data/gridSmall.jpg") 
  id = ImageData.load(Input.new(is)) 
  tex2d.setImage(id) 
  matrix = Matrix.new 
  MatrixBuilder.euclidean.scale(10.0, 5.0, 1.0).assignTo(matrix) 
  tex2d.setTextureMatrix(matrix) 
  v = JRViewer.display(myscene)
end 
end 
MainWindow.new 
