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


package de.jreality.portal;


import java.awt.Color;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.util.Rectangle3D;

/**
 * @author weissman
 *
 */
public class DebugLattice  {

	public static void main(String[] args) {
		SceneGraphComponent grid = makeWorld();
		ViewerApp.display(grid);

	}

	public  static SceneGraphComponent makeWorld()	{
		SceneGraphComponent theWorld = new SceneGraphComponent();
		theWorld.setTransformation(new Transformation());
		theWorld.setName("navComp");
		double scale = PortalCoordinateSystem.xDimPORTAL/2; //4.068*0.3048;  // 2.034; //2*4.068/((dim-1));
		double yscale = PortalCoordinateSystem.yDimPORTAL;  //6.561*0.3048; // 1.64; //6.561/(dim-1);
		double yoffset = PortalCoordinateSystem.yOffsetPORTAL; //1.365*0.3048;
		double[][] bnds = {{-scale, yoffset, -scale},{scale, yoffset+yscale, scale}};
		Rectangle3D portalBox = new Rectangle3D(bnds);
		SceneGraphComponent lattice = makeLattice(portalBox, 5); //-scale*(dim-1)/2., -yscale*(dim-1)/2., -scale*(dim-1)/2., scale, yscale, scale, dim, dim, dim, 0.05, 0.01);
		theWorld.setAppearance(new Appearance());
		theWorld.addChild(lattice);
		return theWorld;
	}
	
    public static SceneGraphComponent makeLattice(Rectangle3D box, int segments) {
        SceneGraphComponent latticeComp = new SceneGraphComponent();
        Appearance ap = new Appearance();
        ap.setAttribute(CommonAttributes.TUBE_RADIUS, .02);
        ap.setAttribute(CommonAttributes.POINT_RADIUS, .04);
        ap.setAttribute(CommonAttributes.VERTEX_DRAW, true);
        ap.setAttribute(CommonAttributes.EDGE_DRAW, true);
        ap.setAttribute(CommonAttributes.SPHERES_DRAW, true);
        ap.setAttribute(CommonAttributes.TUBES_DRAW, true);
        ap.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+".diffuseColor", new Color(.8f, .4f, 0f));
        ap.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+".diffuseColor", new Color(.3f, .8f, 0f));
        latticeComp.setAppearance(ap);
        int total = segments*segments*segments;
        int n = segments;
        int n2 = segments*segments;
        double factor = segments - 1.0;
        double[][] bnds = box.getBounds();
        double[] extent = box.getExtent();
        double[][] verts = new double[total][3];
        int[][] edges = new int[3*n2][2];
        for (int i = 0; i<segments; ++i)	{		// z loop
        	for (int j = 0; j<segments; ++j)	{	// y loop
        		for (int k = 0; k<segments; ++k)	{	// x loop
        			int[] lookup = {i,j,k};
        			for (int m = 0; m<3; ++m)	{
            			verts[i*n2+j*n+k][m] = bnds[0][m] + (lookup[m]/factor)*extent[m];	
        			}
        		}
        	}
        }
        for (int i = 0; i<segments; ++i)	{
        	for (int j = 0; j<segments; ++j)	{	
        		edges[i*n+j][0] = i*n+j;
        		edges[i*n+j][1] = i*n+j+n2*(n-1);
        		edges[i*n+j+n2][0] = i*n2+j;
        		edges[i*n+j+n2][1] = i*n2+j+n*(n-1);
        		edges[i*n+j+2*n2][0] = i*n2+j*n;
        		edges[i*n+j+2*n2][1] = i*n2+j*n+(n-1);
        	}
        }
        IndexedLineSetFactory ifsf = new IndexedLineSetFactory();
        ifsf.setVertexCount(total);
        ifsf.setVertexCoordinates(verts);
        ifsf.setEdgeCount(edges.length);
        ifsf.setEdgeIndices(edges);
        ifsf.update();
        IndexedLineSet ils = ifsf.getIndexedLineSet();
        latticeComp.setGeometry(ils);
        return latticeComp;
    }
    

}
