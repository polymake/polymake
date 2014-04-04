/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.geometry;

import java.awt.Color;

import de.jreality.math.FactoredMatrix;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;


public class CoordinateSystemTest {

		
  /**
   * for testing
   * (add axes or box as children of a given SceneGraphComponent)
   */
  public static void main(String[] args) {
    
    //create a component
//  SceneGraphComponent component = SphereUtility.tessellatedCubeSphere(2); component.setName("Sphere");
	SceneGraphComponent component = parametricSurface();

    FactoredMatrix trans = new FactoredMatrix();
    trans.setRotation(Math.PI, 1, 1, 1);
    //trans.setTranslation(1,0,0);
    trans.assignTo(component);

    //create coordinate system
    final CoordinateSystemFactory coords = new CoordinateSystemFactory(component);
    //SET PROPERTIES:
    //coords.setAxisScale(0.25);
    //coords.setLabelScale(0.005);
    //coords.showBoxArrows(true);
    //coords.showAxesArrows(false);
    //coords.showLabels(false);
    //coords.setColor(Color.RED);
    coords.setGridColor(Color.GRAY);
    //coords.setLabelColor(Color.MAGENTA);
    //coords.setLabelFont(new Font("Sans Serif", Font.PLAIN, 72));
    
    
    //display axes/box/grid
    //coords.showAxes(false);
    coords.showBox(true);
    //coords.showGrid(false);
    
    //beautify box
    coords.beautify(true);
    
    
    //display component
    JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(component);
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();
  }
  
  
  
  private static SceneGraphComponent parametricSurface() {

	  ParametricSurfaceFactory.Immersion immersion = new ParametricSurfaceFactory.Immersion() {

			public void evaluate(double u, double v, double[] xyz, int index) {
				double x = u;
				double y = v;
				
  //edit this line:
				double z = u*v;

				xyz[index + 0] = x;
				xyz[index + 1] = y;
				xyz[index + 2] = z;
			}

			public int getDimensionOfAmbientSpace() {
				return 3;
			}

			public boolean isImmutable() {
				return false;
			}
	  };
	  
	  ParametricSurfaceFactory psf = new ParametricSurfaceFactory();
	  //psf.setMeshSize(5, 5);
	  psf.setGenerateVertexNormals( true );
	  psf.setGenerateEdgesFromFaces( true );

	  psf.setUMin( -1 );
	  psf.setUMax(  2 );
	  psf.setVMin( -2 );
	  psf.setVMax(  1 );

	  psf.setImmersion( immersion );
	  psf.update();
	  
	  SceneGraphComponent parametricSurface = new SceneGraphComponent();
	  parametricSurface.setName("ParametricSurface");
	  Geometry geom = psf.getIndexedFaceSet();
	  geom.setName("parametricSurface");
	  parametricSurface.setGeometry(geom);
	  
	  return parametricSurface;
  }

}