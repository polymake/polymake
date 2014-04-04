/*
 * Created on 16.11.2006
 *
 * This file is part of the de.jreality.soft package.
 * 
 * This program is free software; you can redistribute and/or modify 
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation; either version 2 of the license, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITTNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the 
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307
 * USA 
 */
package de.jreality.soft;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;

public class Test4coords {

    /**
     * @param args
     */
    public static void main(String[] args) {


        IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
        double w = 0.00;
        double [][] vertices  = new double[][] {
          {0, 0, 0,1}, {1, 0, 0,w}, {0, 1, 0,w}, {-1,0,0,w},
          {0,-1,0, w}
        };
        
        int [][] faceIndices = new int [][] {
          {0,1, 2},
          {0, 2, 3},
          {0, 3, 4}, 
          {0, 4, 1}
        };
        
        ifsf.setVertexCount( vertices.length );
        ifsf.setVertexCoordinates( vertices );
        ifsf.setFaceCount( faceIndices.length);
        ifsf.setFaceIndices( faceIndices );
        
        ifsf.setGenerateEdgesFromFaces( true );
        ifsf.setGenerateFaceNormals( true );

        ifsf.update();
        SceneGraphComponent root = new SceneGraphComponent();
        root.setGeometry(ifsf.getIndexedFaceSet());
        SceneGraphComponent cube = new SceneGraphComponent();
        cube.setGeometry(Primitives.coloredCube());
        root.addChild(cube);
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(root);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
    }

}
